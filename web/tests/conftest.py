import os
import time
import signal
import subprocess
import pytest


@pytest.fixture
def engine_path():
    """Path to the boudica binary."""
    return os.path.join(os.path.dirname(__file__), "..", "..", "boudica")


@pytest.fixture(scope="session")
def e2e_server():
    """Start a uvicorn server for E2E tests, shut down after session."""
    port = 8111  # Use a dedicated port to avoid conflicts
    proc = subprocess.Popen(
        [
            "python3", "-m", "uvicorn",
            "web.backend.main:app",
            "--host", "127.0.0.1",
            "--port", str(port),
        ],
        cwd=os.path.join(os.path.dirname(__file__), "..", ".."),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    # Wait for server to be ready
    import urllib.request
    for _ in range(40):
        try:
            resp = urllib.request.urlopen(f"http://127.0.0.1:{port}/health")
            if resp.status == 200:
                break
        except Exception:
            pass
        time.sleep(0.25)
    else:
        proc.kill()
        raise RuntimeError("E2E server failed to start")

    yield f"http://127.0.0.1:{port}"

    # Teardown
    proc.send_signal(signal.SIGTERM)
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
