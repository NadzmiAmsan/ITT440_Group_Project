FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    gcc \
    libmariadb-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY server.c .

RUN gcc -o server server.c \
    $(pkg-config --cflags --libs libmariadb) \
    -lpthread

CMD ["./server"]
