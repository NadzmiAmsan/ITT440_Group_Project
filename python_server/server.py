import os
import socket
import threading
import mysql.connector
import time
from datetime import datetime, timezone, timedelta


HOST = '0.0.0.0'
PORT = 5002              # Chef Py-1's station "door number"

ORDER_BOOK_CONFIG = {
    'host':     'database',
    'port':     3306,
    'user':     'itt440_user',
    'password': 'itt440_pass',
    'database': 'itt440_db'
}

CHEF_NAME = os.getenv("CHEF_NAME", "").strip() or "Chef"

# MySQL stores timestamps in the container's SYSTEM timezone, which we've
# confirmed is UTC. We convert to GMT+8 (Asia/Kuala_Lumpur) for display.
KL_TZ = timezone(timedelta(hours=8))


def to_kl_time(raw_ts):
    """Convert a naive UTC datetime from MySQL into a formatted GMT+8 KL string."""
    if raw_ts is None:
        return "N/A"
    kl_ts = raw_ts.replace(tzinfo=timezone.utc).astimezone(KL_TZ)
    return kl_ts.strftime("%d-%m-%Y %H:%M:%S")


def open_order_book():
    """Connect to the Order Book. Retry every 3s if it's not ready yet."""
    while True:
        try:
            return mysql.connector.connect(**ORDER_BOOK_CONFIG)
        except mysql.connector.Error as e:
            print(f"[Order Book] Not ready yet, retrying in 3s... ({e})")
            time.sleep(3)


def resolve_chef_name():
    configured_name = CHEF_NAME
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT user
        FROM socket_data
        WHERE user = %s
    """, (configured_name,))
    row = cursor.fetchone()
    cursor.close()
    conn.close()
    if row and row[0]:
        return row[0]
    return configured_name


def cook_one_dish():
    """Background timer: cooks (and logs) one dish every 30 seconds."""
    while True:
        try:
            chef_name = resolve_chef_name()
            conn = open_order_book()
            cursor = conn.cursor()
            cursor.execute("""
                INSERT INTO socket_data (user, points, datetime_stamp)
                VALUES (%s, 1, NOW())
                ON DUPLICATE KEY UPDATE
                    points         = points + 1,
                    datetime_stamp = NOW()
            """, (chef_name,))
            conn.commit()
            cursor.close()
            conn.close()
            print(f"[{to_kl_time(datetime.utcnow())} GMT+8] 🍳 {chef_name} cooked a dish! Logged in Order Book.")
        except Exception as e:
            print(f"[Order Book ERROR] {e}")
        time.sleep(30)


# ── Helper: GET_NAME ────────────────────────────────────────
def handle_get_name():
    return resolve_chef_name()


# ── Command 1: GET_POINTS ──────────────────────────────────────
def handle_get_points():
    chef_name = resolve_chef_name()
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT user, points, datetime_stamp
        FROM socket_data WHERE user = %s
    """, (chef_name,))
    row = cursor.fetchone()
    cursor.close()
    conn.close()
    if row:
        return f"I have cooked {row[1]} dishes so far! My last dish was served at {to_kl_time(row[2])} (GMT+8 KL)."
    return "I haven't cooked anything yet — no record found."


# ── Command 2: GET_HISTORY ─────────────────────────────────────
def handle_get_history():
    chef_name = resolve_chef_name()
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT points_before, points_after, updated_at
        FROM socket_data_history
        WHERE user = %s
        ORDER BY updated_at DESC
        LIMIT 5
    """, (chef_name,))
    rows = cursor.fetchall()
    cursor.close()
    conn.close()

    if not rows:
        return "I haven't logged any dishes yet — my kitchen activity log is empty!"

    lines = ["Here are my last 5 dish updates:"]
    for before, after, ts in rows:
        lines.append(f"  I went from {before} to {after} dishes @ {to_kl_time(ts)} (GMT+8 KL)")
    return "\n".join(lines)


# ── Command 3: GET_RANK ────────────────────────────────────────
def handle_get_rank():
    chef_name = resolve_chef_name()
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT `rank`, points
        FROM leaderboard
        WHERE user = %s
    """, (chef_name,))
    row = cursor.fetchone()
    cursor.close()
    conn.close()
    if row:
        return f"I am currently ranked #{row[0]} on the Top Chef Board, with {row[1]} dishes cooked!"
    return "I am not ranked yet — I haven't cooked any dishes."


# ── Command 4: GET_TIME ────────────────────────────────────────
def handle_get_time():
    chef_name = resolve_chef_name()
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT datetime_stamp
        FROM socket_data WHERE user = %s
    """, (chef_name,))
    row = cursor.fetchone()
    cursor.close()
    conn.close()
    if row:
        return f"My last dish was cooked at {to_kl_time(row[0])} (GMT+8 KL)."
    return "I haven't cooked any dishes yet — no time to report."


# ── Command 5: ADD_POINTS ──────────────────────────────────────
def handle_add_points(amount):
    chef_name = resolve_chef_name()
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        INSERT INTO socket_data (user, points, datetime_stamp)
        VALUES (%s, 0, NOW())
        ON DUPLICATE KEY UPDATE
            points = points + %s,
            datetime_stamp = NOW()
    """, (chef_name, amount))
    conn.commit()
    cursor.close()
    conn.close()
    return f"Added {amount} extra dish point(s) to {chef_name}."


# ── Command Router ──────────────────────────────────────────────
def normalize_command(command):
    """Turn 1-5 input into the matching server command."""
    normalized = command.strip().upper()
    if normalized.startswith("ADD_POINTS"):
        return "ADD_POINTS"
    mapping = {
        "1": "GET_POINTS",
        "2": "GET_HISTORY",
        "3": "GET_RANK",
        "4": "GET_TIME",
        "5": "ADD_POINTS",
    }
    return mapping.get(normalized, normalized)


COMMANDS = {
    "GET_POINTS":  handle_get_points,
    "GET_HISTORY": handle_get_history,
    "GET_RANK":    handle_get_rank,
    "GET_TIME":    handle_get_time,
    "GET_NAME":    handle_get_name,
    "ADD_POINTS":  handle_add_points,
}


def serve_waiter(conn, addr):
    """Handle one Waiter's questions at the counter."""
    print(f"[+] A Waiter approached the counter: {addr}")
    try:
        while True:
            data = conn.recv(1024).decode('utf-8').strip()
            if not data:
                break

            command = normalize_command(data)
            print(f"[Waiter asked] {command}")

            if command == "ADD_POINTS":
                try:
                    amount = int(data.split()[-1])
                except ValueError:
                    amount = 0
                if amount <= 0:
                    response = "Please use: ADD_POINTS <number>"
                else:
                    response = COMMANDS[command](amount)
            elif command in COMMANDS:
                response = COMMANDS[command]()
            else:
                chef_name = resolve_chef_name()
                response = (
                    f"{chef_name} doesn't understand. Try one of:\n"
                    "  1, 2, 3, 4, 5"
                )

            conn.sendall((response + "\n").encode('utf-8'))
    except Exception as e:
        print(f"[ERROR] {e}")
    finally:
        conn.close()
        print(f"[-] Waiter left the counter: {addr}")


def main():
    timer_thread = threading.Thread(target=cook_one_dish, daemon=True)
    timer_thread.start()

    counter = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    counter.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    counter.bind((HOST, PORT))
    counter.listen(5)
    print(f"[{resolve_chef_name()}] 👨‍🍳 Kitchen station open on port {PORT}. Ready to cook!")
    print(f"[{resolve_chef_name()}] Accepted commands: 1, 2, 3, 4")

    while True:
        conn, addr = counter.accept()
        threading.Thread(target=serve_waiter, args=(conn, addr), daemon=True).start()


if __name__ == '__main__':
    main()