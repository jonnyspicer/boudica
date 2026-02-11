"""FastAPI backend for Boudica chess web interface."""
import json
import uuid
import asyncio
from contextlib import asynccontextmanager

import chess
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.responses import JSONResponse
import os

from .engine import UCIEngine
from .game import GameManager

# Active games: game_id -> (GameManager, UCIEngine)
games: dict[str, tuple[GameManager, UCIEngine]] = {}

FRONTEND_DIR = os.path.join(os.path.dirname(__file__), "..", "frontend")


@asynccontextmanager
async def lifespan(app: FastAPI):
    yield
    # Cleanup: stop all engines
    for gid, (_, engine) in list(games.items()):
        await engine.stop()
    games.clear()


app = FastAPI(title="Boudica Chess", lifespan=lifespan)


@app.get("/health")
async def health():
    return {"status": "ok", "engine": "boudica", "games_active": len(games)}


@app.post("/api/games")
async def create_game():
    game_id = str(uuid.uuid4())
    return JSONResponse(
        {"game_id": game_id, "ws_url": f"/ws/game/{game_id}"},
        status_code=201,
    )


@app.websocket("/ws/game/{game_id}")
async def websocket_game(websocket: WebSocket, game_id: str):
    await websocket.accept()

    try:
        while True:
            raw = await websocket.receive_text()
            try:
                msg = json.loads(raw)
            except json.JSONDecodeError:
                await _send(websocket, {
                    "type": "error",
                    "code": "invalid_json",
                    "message": "Invalid JSON",
                })
                continue

            msg_type = msg.get("type")

            if msg_type == "new_game":
                await _handle_new_game(websocket, game_id, msg)
            elif msg_type == "player_move":
                await _handle_player_move(websocket, game_id, msg)
            elif msg_type == "resign":
                await _handle_resign(websocket, game_id)
            elif msg_type == "offer_draw":
                await _handle_draw(websocket, game_id)
            elif msg_type == "request_state":
                await _handle_request_state(websocket, game_id)
            else:
                await _send(websocket, {
                    "type": "error",
                    "code": "unknown_message",
                    "message": "Unknown message type",
                })

    except WebSocketDisconnect:
        pass
    finally:
        if game_id in games:
            _, engine = games.pop(game_id)
            await engine.stop()


async def _handle_new_game(websocket: WebSocket, game_id: str, msg: dict):
    # Stop existing game if any
    if game_id in games:
        _, old_engine = games.pop(game_id)
        await old_engine.stop()

    skill_level = max(0, min(20, int(msg.get("skill_level", 10))))
    raw_time_ms = int(msg.get("time_ms", 300000))
    time_ms = 0 if raw_time_ms == 0 else max(10000, min(3600000, raw_time_ms))
    increment_ms = max(0, min(60000, int(msg.get("increment_ms", 3000))))
    player_color = msg.get("player_color", "white")
    if player_color not in ("white", "black"):
        player_color = "white"

    game = GameManager(
        player_color=player_color,
        skill_level=skill_level,
        time_ms=time_ms,
        increment_ms=increment_ms,
    )
    game.game_id = game_id

    engine = UCIEngine()
    await engine.start(hash_mb=64, skill_level=skill_level)
    await engine.new_game()

    games[game_id] = (game, engine)

    await _send(websocket, {
        "type": "game_started",
        "game_id": game_id,
        "player_color": player_color,
        "fen": game.fen,
        "clocks": game.clock.to_dict(),
    })

    # If engine plays first (player is black), make engine move
    if game.is_engine_turn:
        await _do_engine_move(websocket, game_id)


async def _handle_player_move(websocket: WebSocket, game_id: str, msg: dict):
    if game_id not in games:
        await _send(websocket, {
            "type": "error",
            "code": "no_game",
            "message": "No active game. Send new_game first.",
        })
        return

    game, engine = games[game_id]
    uci = msg.get("uci", "")

    ok, san, error = game.try_player_move(uci)
    if not ok:
        if error == "flag_fallen" and game.result:
            await _send(websocket, {
                "type": "game_over",
                "result": game.result,
                "termination": game.termination,
                "fen": game.fen,
            })
        else:
            await _send(websocket, {
                "type": "error",
                "code": error,
                "message": error,
            })
        return

    # Send player move confirmation - the color that just moved
    moved_color = "black" if game.board.turn == chess.WHITE else "white"

    await _send(websocket, {
        "type": "move_made",
        "color": moved_color,
        "uci": uci,
        "san": san,
        "fen": game.fen,
        "is_player_turn": game.is_player_turn,
        "clocks": game.clock.to_dict(),
    })

    if game.result:
        await _send(websocket, {
            "type": "game_over",
            "result": game.result,
            "termination": game.termination,
            "fen": game.fen,
        })
        return

    await _do_engine_move(websocket, game_id)


async def _do_engine_move(websocket: WebSocket, game_id: str):
    if game_id not in games:
        return

    game, engine = games[game_id]

    await engine.set_position(game.moves if game.moves else None)

    async def info_callback(info):
        await _send(websocket, {
            "type": "engine_thinking",
            "depth": info.depth,
            "score_cp": info.score_cp,
            "score_mate": info.score_mate,
            "pv": info.pv,
        })

    clock_params = game.get_engine_clock_params()
    bestmove, _ = await engine.search(
        wtime=clock_params["wtime"],
        btime=clock_params["btime"],
        winc=clock_params["winc"],
        binc=clock_params["binc"],
        info_callback=info_callback,
    )

    if bestmove in ("0000", "(none)"):
        return

    san = game.apply_engine_move(bestmove)
    engine_color = "white" if game.engine_color else "black"

    await _send(websocket, {
        "type": "move_made",
        "color": engine_color,
        "uci": bestmove,
        "san": san,
        "fen": game.fen,
        "is_player_turn": game.is_player_turn,
        "clocks": game.clock.to_dict(),
    })

    if game.result:
        await _send(websocket, {
            "type": "game_over",
            "result": game.result,
            "termination": game.termination,
            "fen": game.fen,
        })


async def _handle_resign(websocket: WebSocket, game_id: str):
    if game_id not in games:
        return
    game, _ = games[game_id]
    game.resign(game.player_color)
    await _send(websocket, {
        "type": "game_over",
        "result": game.result,
        "termination": game.termination,
        "fen": game.fen,
    })


async def _handle_draw(websocket: WebSocket, game_id: str):
    if game_id not in games:
        return
    game, _ = games[game_id]
    accepted = game.offer_draw()
    if not accepted:
        await _send(websocket, {"type": "draw_declined"})


async def _handle_request_state(websocket: WebSocket, game_id: str):
    if game_id not in games:
        await _send(websocket, {
            "type": "error",
            "code": "no_game",
            "message": "No active game.",
        })
        return
    game, _ = games[game_id]
    await _send(websocket, {
        "type": "game_started",
        "game_id": game_id,
        "player_color": "white" if game.player_color else "black",
        "fen": game.fen,
        "clocks": game.clock.to_dict(),
    })


async def _send(ws: WebSocket, data: dict):
    await ws.send_text(json.dumps(data))


# Serve frontend static files - must be last (skipped if directory missing, e.g. in Docker)
if os.path.isdir(FRONTEND_DIR):
    app.mount("/", StaticFiles(directory=FRONTEND_DIR, html=True), name="frontend")
