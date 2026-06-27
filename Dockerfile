# =================================================================
# THE DOCKER DINER - Chef C's Kitchen Station (Server Recipe)
# =================================================================
# This Dockerfile builds the sealed kitchen station for Chef C.
# It installs everything Chef C needs: a C compiler and the
# MySQL connector library so Chef C can talk to the Order Book.
# =================================================================

FROM debian:bookworm-slim

# Install gcc (the compiler) and MariaDB client library
# (Debian Bookworm renamed libmysqlclient-dev to libmariadb-dev)
RUN apt-get update && apt-get install -y \
    gcc \
    libmariadb-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY server.c .

# Compile Chef C's recipe (server.c) into a runnable program
RUN gcc -o server server.c \
    $(pkg-config --cflags --libs libmariadb) \
    -lpthread

# Open the kitchen station for business
CMD ["./server"]
