"""UCI Engine subprocess wrapper for Boudica."""
import asyncio
import os
import re
from dataclasses import dataclass, field

# Path to the boudica binary (relative to project root)
ENGINE_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "boudica")


@dataclass
class EngineInfo:
    depth: int = 0
    seldepth: int = 0
    score_cp: int | None = None
    score_mate: int | None = None
    nodes: int = 0
    nps: int = 0
    time_ms: int = 0
    pv: list[str] = field(default_factory=list)


class UCIEngine:
    """Manages a UCI engine subprocess.

    Uses asyncio.create_subprocess_exec (not shell) to safely spawn
    the engine binary with no injection risk.
    """

    def __init__(self, engine_path: str = ENGINE_PATH):
        self.engine_path = os.path.abspath(engine_path)
        self.process: asyncio.subprocess.Process | None = None
        self._read_lock = asyncio.Lock()

    async def start(self, hash_mb: int = 64, skill_level: int = 20) -> None:
        """Start engine process, do UCI handshake, configure options."""
        # create_subprocess_exec: safe, no shell involved
        self.process = await asyncio.create_subprocess_exec(
            self.engine_path,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        # UCI handshake
        await self._send("uci")
        await self._read_until("uciok")

        # Set options (values are validated ints, not user strings)
        hash_mb = max(1, min(1024, int(hash_mb)))
        skill_level = max(0, min(20, int(skill_level)))
        await self._send(f"setoption name Hash value {hash_mb}")
        await self._send(f"setoption name Skill Level value {skill_level}")
        await self._send("isready")
        await self._read_until("readyok")

    async def new_game(self) -> None:
        await self._send("ucinewgame")
        await self._send("isready")
        await self._read_until("readyok")

    async def set_position(self, moves: list[str] | None = None) -> None:
        if moves:
            # Validate: each move must be 4-5 alphanumeric chars
            safe_moves = [m for m in moves if re.match(r'^[a-h][1-8][a-h][1-8][qrbn]?$', m)]
            move_str = " ".join(safe_moves)
            await self._send(f"position startpos moves {move_str}")
        else:
            await self._send("position startpos")

    async def search(
        self,
        wtime: int = 0,
        btime: int = 0,
        winc: int = 0,
        binc: int = 0,
        movetime: int = 0,
        info_callback=None,
    ) -> tuple[str, list[EngineInfo]]:
        """Run search, return (bestmove, list of info updates)."""
        if movetime > 0:
            await self._send(f"go movetime {int(movetime)}")
        else:
            await self._send(
                f"go wtime {int(wtime)} btime {int(btime)} "
                f"winc {int(winc)} binc {int(binc)}"
            )

        infos = []
        bestmove = "0000"

        async with self._read_lock:
            while True:
                line = await self._readline()
                if line is None:
                    break

                if line.startswith("bestmove"):
                    parts = line.split()
                    bestmove = parts[1] if len(parts) >= 2 else "0000"
                    break
                elif line.startswith("info") and "depth" in line:
                    info = self._parse_info(line)
                    infos.append(info)
                    if info_callback:
                        await info_callback(info)

        return bestmove, infos

    async def stop_search(self) -> None:
        """Send 'stop' to interrupt an ongoing search."""
        try:
            await self._send("stop")
        except (BrokenPipeError, ConnectionResetError):
            pass

    async def stop(self) -> None:
        """Stop the engine process."""
        if self.process and self.process.returncode is None:
            try:
                await self._send("stop")
                await self._send("quit")
                try:
                    await asyncio.wait_for(self.process.wait(), timeout=2.0)
                except asyncio.TimeoutError:
                    self.process.kill()
                    await self.process.wait()
            except (BrokenPipeError, ConnectionResetError):
                if self.process.returncode is None:
                    self.process.kill()
                    await self.process.wait()
        self.process = None

    async def _send(self, cmd: str) -> None:
        if self.process and self.process.stdin:
            self.process.stdin.write(f"{cmd}\n".encode())
            await self.process.stdin.drain()

    async def _readline(self) -> str | None:
        if self.process and self.process.stdout:
            try:
                line = await asyncio.wait_for(
                    self.process.stdout.readline(), timeout=30.0
                )
                return line.decode().strip() if line else None
            except asyncio.TimeoutError:
                return None
        return None

    async def _read_until(self, target: str) -> list[str]:
        lines = []
        async with self._read_lock:
            while True:
                line = await self._readline()
                if line is None:
                    break
                lines.append(line)
                if target in line:
                    break
        return lines

    @staticmethod
    def _parse_info(line: str) -> EngineInfo:
        info = EngineInfo()

        depth_m = re.search(r"\bdepth (\d+)", line)
        if depth_m:
            info.depth = int(depth_m.group(1))

        seldepth_m = re.search(r"\bseldepth (\d+)", line)
        if seldepth_m:
            info.seldepth = int(seldepth_m.group(1))

        mate_m = re.search(r"\bscore mate (-?\d+)", line)
        cp_m = re.search(r"\bscore cp (-?\d+)", line)
        if mate_m:
            info.score_mate = int(mate_m.group(1))
        elif cp_m:
            info.score_cp = int(cp_m.group(1))

        nodes_m = re.search(r"\bnodes (\d+)", line)
        if nodes_m:
            info.nodes = int(nodes_m.group(1))

        nps_m = re.search(r"\bnps (\d+)", line)
        if nps_m:
            info.nps = int(nps_m.group(1))

        time_m = re.search(r"\btime (\d+)", line)
        if time_m:
            info.time_ms = int(time_m.group(1))

        pv_m = re.search(r"\bpv (.+)$", line)
        if pv_m:
            info.pv = pv_m.group(1).split()

        return info
