# ITT440 Network Programming — Group Project

## Group Members
| Name | Task |
|------|------|
| Nadzmi | Database + Python Server 2 + Python Client 2 |
| Teammate 1 | C Server + C Client |
| Teammate 2 | Python Server 1 + Python Client 1 |

---

## Project Structure
```
ITT440_Group_Project/
├── Dockerfile              ← Database image
├── init.sql                ← Database setup (tables, trigger, view)
├── docker-compose.yml      ← All containers
├── python_server2/         ← Nadzmi's Python server (port 5003)
├── python_client2/         ← Nadzmi's Python client
├── c_server/               ← Teammate 1's C server (port 5001)
├── c_client/               ← Teammate 1's C client
├── python_server/          ← Teammate 2's Python server (port 5002)
└── python_client/          ← Teammate 2's Python client
```

---

## Database Details
| Setting | Value |
|---------|-------|
| Host (inside Docker) | `database` |
| Port | `3306` |
| Database | `itt440_db` |
| Username | `itt440_user` |
| Password | `itt440_pass` |

## Tables
| Table | Purpose |
|-------|---------|
| `socket_data` | Stores user, points, datetime_stamp |
| `socket_data_history` | Audit trail of every points update |

## View
| View | Purpose |
|------|---------|
| `leaderboard` | Ranked list of all servers by points |

---

## Port Assignments
| Container | Port |
|-----------|------|
| Database | 3306 (internal) / 3307 (host) |
| C Server | 5001 |
| Python Server 1 | 5002 |
| Python Server 2 | 5003 |

---

## How to Run

### Database only (Nadzmi)
```bash
docker-compose up -d database
```

### All containers (after everyone pushes their code)
```bash
docker-compose up -d database c_server python_server1 python_server2
```

### Test clients
```bash
docker-compose run --rm c_client
docker-compose run --rm python_client1
docker-compose run --rm python_client2
```

### Verify database
```bash
# Show all tables
docker exec -it itt440_database mysql -u itt440_user -pitt440_pass itt440_db -e "SHOW TABLES;"

# Show leaderboard
docker exec -it itt440_database mysql -u itt440_user -pitt440_pass itt440_db -e "SELECT * FROM leaderboard;"
```
