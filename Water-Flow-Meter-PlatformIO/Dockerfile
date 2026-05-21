FROM debian:12-slim

ENV DEBIAN_FRONTEND=noninteractive \
    PLATFORMIO_CORE_DIR=/pio \
    PLATFORMIO_GLOBALLIB_DIR=/pio/libdeps \
    PLATFORMIO_CACHE_DIR=/pio/cache

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 python3-pip python3-venv git ca-certificates build-essential \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --no-cache-dir --break-system-packages platformio

WORKDIR /workspace
COPY platformio.ini ./
RUN pio pkg install

COPY . .

CMD ["pio", "run"]
