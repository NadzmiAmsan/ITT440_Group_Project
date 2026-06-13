# =============================================================
# ITT440 Network Programming - Group Project
# Database Container
# Author  : Nadzmi
# Base    : MySQL 8.0 (official Docker Hub image)
# Purpose : Runs the MySQL database for the socket servers
# =============================================================

FROM mysql:8.0

# ── Labels (metadata visible in docker inspect) ──────────────
LABEL maintainer="Nadzmi"
LABEL project="ITT440 Network Programming"
LABEL description="MySQL database container for socket server leaderboard"
LABEL version="2.0"

# ── Environment variables ─────────────────────────────────────
ENV MYSQL_ROOT_PASSWORD=rootpassword \
    MYSQL_DATABASE=itt440_db         \
    MYSQL_USER=itt440_user           \
    MYSQL_PASSWORD=itt440_pass

# ── MySQL performance tuning ──────────────────────────────────
# Written to a custom config file inside the container
RUN echo "[mysqld]"                          >> /etc/mysql/conf.d/itt440.cnf && \
    echo "character-set-server=utf8mb4"      >> /etc/mysql/conf.d/itt440.cnf && \
    echo "collation-server=utf8mb4_unicode_ci" >> /etc/mysql/conf.d/itt440.cnf && \
    echo "max_connections=100"               >> /etc/mysql/conf.d/itt440.cnf && \
    echo "wait_timeout=600"                  >> /etc/mysql/conf.d/itt440.cnf && \
    echo "event_scheduler=ON"               >> /etc/mysql/conf.d/itt440.cnf

# ── Init script ───────────────────────────────────────────────
# MySQL runs all *.sql files in this folder automatically
# on first container start (only when data volume is empty)
COPY init.sql /docker-entrypoint-initdb.d/01_init.sql

# ── Port ──────────────────────────────────────────────────────
EXPOSE 3306
