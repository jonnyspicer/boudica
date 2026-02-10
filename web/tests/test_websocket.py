"""Tests for WebSocket game flow via FastAPI TestClient."""
import json
import pytest
from fastapi.testclient import TestClient
from web.backend.main import app


class TestWebSocketFlow:
    def test_new_game_white(self):
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-1") as ws:
            ws.send_text(json.dumps({
                "type": "new_game",
                "skill_level": 1,
                "time_ms": 60000,
                "increment_ms": 0,
                "player_color": "white"
            }))
            msg = json.loads(ws.receive_text())
            assert msg["type"] == "game_started"
            assert msg["player_color"] == "white"
            assert "fen" in msg
            assert "clocks" in msg

    def test_new_game_black_engine_moves_first(self):
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-2") as ws:
            ws.send_text(json.dumps({
                "type": "new_game",
                "skill_level": 1,
                "time_ms": 60000,
                "increment_ms": 0,
                "player_color": "black"
            }))
            # Should get game_started
            msg = json.loads(ws.receive_text())
            assert msg["type"] == "game_started"
            assert msg["player_color"] == "black"

            # Engine should think and move
            messages = []
            for _ in range(20):  # Read up to 20 messages
                try:
                    raw = ws.receive_text()
                    m = json.loads(raw)
                    messages.append(m)
                    if m["type"] == "move_made":
                        break
                except Exception:
                    break

            types = [m["type"] for m in messages]
            assert "move_made" in types

    def test_player_move_and_engine_response(self):
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-3") as ws:
            ws.send_text(json.dumps({
                "type": "new_game",
                "skill_level": 1,
                "time_ms": 60000,
                "increment_ms": 0,
                "player_color": "white"
            }))
            json.loads(ws.receive_text())  # game_started

            # Send player move
            ws.send_text(json.dumps({
                "type": "player_move",
                "uci": "e2e4"
            }))

            # Should get move confirmation + engine thinking + engine move
            messages = []
            for _ in range(30):
                try:
                    raw = ws.receive_text()
                    m = json.loads(raw)
                    messages.append(m)
                    # Stop after engine's move_made
                    if m["type"] == "move_made" and m.get("color") != "white":
                        break
                except Exception:
                    break

            types = [m["type"] for m in messages]
            assert "move_made" in types
            # First move_made should be our move confirmation
            first_move = next(m for m in messages if m["type"] == "move_made")
            assert first_move["san"] == "e4"

    def test_illegal_move(self):
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-4") as ws:
            ws.send_text(json.dumps({
                "type": "new_game",
                "skill_level": 1,
                "time_ms": 60000,
                "increment_ms": 0,
                "player_color": "white"
            }))
            json.loads(ws.receive_text())

            ws.send_text(json.dumps({
                "type": "player_move",
                "uci": "e2e5"
            }))
            msg = json.loads(ws.receive_text())
            assert msg["type"] == "error"
            assert msg["code"] == "illegal_move"

    def test_resign(self):
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-5") as ws:
            ws.send_text(json.dumps({
                "type": "new_game",
                "skill_level": 1,
                "time_ms": 60000,
                "increment_ms": 0,
                "player_color": "white"
            }))
            json.loads(ws.receive_text())

            ws.send_text(json.dumps({"type": "resign"}))
            msg = json.loads(ws.receive_text())
            assert msg["type"] == "game_over"
            assert msg["result"] == "black_wins"
            assert msg["termination"] == "resignation"

    def test_draw_declined(self):
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-6") as ws:
            ws.send_text(json.dumps({
                "type": "new_game",
                "skill_level": 1,
                "time_ms": 60000,
                "increment_ms": 0,
                "player_color": "white"
            }))
            json.loads(ws.receive_text())

            ws.send_text(json.dumps({"type": "offer_draw"}))
            msg = json.loads(ws.receive_text())
            assert msg["type"] == "draw_declined"

    def test_no_game_error(self):
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-7") as ws:
            ws.send_text(json.dumps({
                "type": "player_move",
                "uci": "e2e4"
            }))
            msg = json.loads(ws.receive_text())
            assert msg["type"] == "error"
            assert msg["code"] == "no_game"

    def test_invalid_json(self):
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-8") as ws:
            ws.send_text("not json at all")
            msg = json.loads(ws.receive_text())
            assert msg["type"] == "error"
            assert msg["code"] == "invalid_json"

    def test_skill_level_bounds(self):
        """Skill level should be clamped to 0-20."""
        client = TestClient(app)
        with client.websocket_connect("/ws/game/test-9") as ws:
            ws.send_text(json.dumps({
                "type": "new_game",
                "skill_level": 99,
                "time_ms": 60000,
                "increment_ms": 0,
                "player_color": "white"
            }))
            msg = json.loads(ws.receive_text())
            assert msg["type"] == "game_started"


class TestHealthEndpoint:
    def test_health(self):
        client = TestClient(app)
        resp = client.get("/health")
        assert resp.status_code == 200
        data = resp.json()
        assert data["status"] == "ok"
        assert data["engine"] == "boudica"


class TestCreateGame:
    def test_create_game(self):
        client = TestClient(app)
        resp = client.post("/api/games")
        assert resp.status_code == 201
        data = resp.json()
        assert "game_id" in data
        assert "ws_url" in data
        assert data["ws_url"].startswith("/ws/game/")
