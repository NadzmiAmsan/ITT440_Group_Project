# ITT440 – Database Container (Your Task)

## Files
```
.
├── Dockerfile          ← builds the MySQL image
├── init.sql            ← creates database & table automatically
├── docker-compose.yml  ← orchestrates all containers (shared with team)
└── README.md
```

---

## Database Details (share with teammates)

| Setting        | Value             |
|----------------|-------------------|
| Host           | `database`        |  ← use this hostname inside Docker network
| Port           | `3306`            |
| Database       | `itt440_db`       |
| Username       | `itt440_user`     |
| Password       | `itt440_pass`     |
| Root Password  | `rootpassword`    |
| Table          | `socket_data`     |

---

## Table Structure

```sql
CREATE TABLE socket_data (
    id             INT AUTO_INCREMENT PRIMARY KEY,
    user           VARCHAR(100) NOT NULL UNIQUE,
    points         INT NOT NULL DEFAULT 0,
    datetime_stamp DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
                   ON UPDATE CURRENT_TIMESTAMP
);
```

Initial rows inserted automatically:
- `c_server_user`      → for C server teammate
- `python_server_user` → for Python server teammate

---

## How to Run (Database Only)

```bash
# 1. Build and start the database container
docker-compose up -d database

# 2. Check it is running
docker ps

# 3. Verify the table was created
docker exec -it itt440_database mysql -u itt440_user -pitt440_pass itt440_db -e "SELECT * FROM socket_data;"
```

---

## How to Run (Full Project – all teammates done)

```bash
# Start everything
docker-compose up -d

# View logs
docker-compose logs -f

# Stop everything
docker-compose down
```

---

## SQL teammates need (UPDATE every 30 seconds)

```sql
-- C server uses this (replace points value):
INSERT INTO socket_data (user, points, datetime_stamp)
VALUES ('c_server_user', 1, NOW())
ON DUPLICATE KEY UPDATE
    points = points + 1,
    datetime_stamp = NOW();

-- Python server uses this:
INSERT INTO socket_data (user, points, datetime_stamp)
VALUES ('python_server_user', 1, NOW())
ON DUPLICATE KEY UPDATE
    points = points + 1,
    datetime_stamp = NOW();

-- Client asks server → server runs this SELECT:
SELECT user, points, datetime_stamp
FROM socket_data
WHERE user = 'c_server_user';
```

---

## Useful Commands

```bash
# Enter MySQL shell inside container
docker exec -it itt440_database mysql -u root -prootpassword itt440_db

# Remove container and volume (reset database)
docker-compose down -v
```
