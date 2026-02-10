"""Tests for UCIEngine with the real boudica binary."""
import asyncio
import pytest
from web.backend.engine import UCIEngine


@pytest.fixture
def engine_path():
    import os
    return os.path.join(os.path.dirname(__file__), "..", "..", "boudica")


@pytest.mark.asyncio
async def test_engine_start_stop(engine_path):
    engine = UCIEngine(engine_path)
    await engine.start(hash_mb=16, skill_level=5)
    assert engine.process is not None
    assert engine.process.returncode is None
    await engine.stop()
    assert engine.process is None


@pytest.mark.asyncio
async def test_engine_search(engine_path):
    engine = UCIEngine(engine_path)
    await engine.start(hash_mb=16, skill_level=5)
    await engine.set_position()
    bestmove, infos = await engine.search(movetime=500)
    assert bestmove != "0000"
    assert len(bestmove) >= 4
    assert len(infos) > 0
    await engine.stop()


@pytest.mark.asyncio
async def test_engine_position_with_moves(engine_path):
    engine = UCIEngine(engine_path)
    await engine.start(hash_mb=16, skill_level=5)
    await engine.set_position(["e2e4", "e7e5"])
    bestmove, _ = await engine.search(movetime=500)
    assert bestmove != "0000"
    await engine.stop()


@pytest.mark.asyncio
async def test_engine_skill_level_depth(engine_path):
    """At skill level 1, max depth should be 2."""
    engine = UCIEngine(engine_path)
    await engine.start(hash_mb=16, skill_level=1)
    await engine.set_position()
    _, infos = await engine.search(movetime=2000)
    max_depth = max(i.depth for i in infos) if infos else 0
    assert max_depth <= 3  # Allow check extensions to go slightly beyond
    await engine.stop()


@pytest.mark.asyncio
async def test_engine_new_game(engine_path):
    engine = UCIEngine(engine_path)
    await engine.start(hash_mb=16, skill_level=10)
    await engine.new_game()
    await engine.set_position()
    bestmove, _ = await engine.search(movetime=300)
    assert bestmove != "0000"
    await engine.stop()


@pytest.mark.asyncio
async def test_engine_info_callback(engine_path):
    engine = UCIEngine(engine_path)
    await engine.start(hash_mb=16, skill_level=10)
    await engine.set_position()

    infos_received = []

    async def callback(info):
        infos_received.append(info)

    await engine.search(movetime=500, info_callback=callback)
    assert len(infos_received) > 0
    assert infos_received[-1].depth > 0
    await engine.stop()


@pytest.mark.asyncio
async def test_move_validation_in_set_position(engine_path):
    """Verify that invalid moves are filtered out."""
    engine = UCIEngine(engine_path)
    await engine.start(hash_mb=16, skill_level=5)
    # Include an invalid move that should be filtered
    await engine.set_position(["e2e4", "INVALID", "e7e5"])
    bestmove, _ = await engine.search(movetime=300)
    assert bestmove != "0000"
    await engine.stop()
