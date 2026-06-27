FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    gcc \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY client.c .

RUN gcc -o client client.c

CMD ["./client"]
