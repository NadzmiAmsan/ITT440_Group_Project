# ITT440 Network Programming — Group Project

This project runs a small restaurant-style network system with multiple chef servers and waiter clients.

## Project structure
```text
ITT440_Group_Project/
├── c_client/            # C waiter client
├── c_server/            # C chef server
├── python_client/       # Python waiter client
├── python_client2/      # Python waiter client
├── python_server/       # Python chef server
├── python_server2/      # Python chef server
├── init.sql             # Database setup
├── docker-compose.yml   # Container setup
└── Dockerfile           # Database image
```

## Database details
- Host: `database`
- Port: `3306`
- Database: `itt440_db`
- User: `itt440_user`
- Password: `itt440_pass`

## Main tables
- `socket_data`: stores each chef's points and last update time
- `socket_data_history`: stores point changes over time
- `leaderboard`: shows the ranked list of chefs

## Run commands
```bash
docker-compose up -d database
```

```bash
docker-compose up -d database c_server python_server1 python_server2
```

```bash
docker-compose run --rm c_client
docker-compose run --rm python_client1
docker-compose run --rm python_client2
```
