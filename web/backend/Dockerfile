# Boudica Backend: C++ chess engine + Python FastAPI server

# Stage 1: Compile the C++ chess engine
FROM debian:bookworm-slim AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends g++ make && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/ ./src/
COPY Makefile ./

RUN make CXX=g++ OPT_FLAGS="-O3 -flto -DNDEBUG"

# Stage 2: Python runtime
FROM python:3.12-slim

WORKDIR /app

COPY web/backend/requirements.txt ./web/backend/
RUN pip install --no-cache-dir -r web/backend/requirements.txt

COPY web/backend/ ./web/backend/
COPY --from=builder /build/boudica ./boudica

RUN useradd -m -u 1001 appuser && \
    chown -R appuser:appuser /app
USER appuser

EXPOSE 8000

CMD ["uvicorn", "web.backend.main:app", "--host", "0.0.0.0", "--port", "8000"]
