"""
=================================================================
 THE DOCKER DINER - Chef Py-1's Kitchen Station (SERVER)
=================================================================
 ELI5: This is Chef Py-1. Every 30 seconds, Chef Py-1 cooks one
 dish and writes it down in the Order Book: "+1 dish!"

 A Waiter (client) can now ask Chef Py-1 FOUR different
 questions, not just one:

   GET_POINTS  -> "How many dishes have you cooked?"
   GET_HISTORY -> "Show me your last 5 dish updates."
   GET_RANK    -> "What's your rank on the Top Chef Board?"
   GET_TIME    -> "When did you cook your last dish?"

 Chef Py-1 works inside their OWN sealed kitchen station
 (Docker container) - completely separate from Chef C and Chef Py-2.
=================================================================
"""

import socket
import threading
import mysql.connector
import time
from datetime import datetime


HOST = '0.0.0.0'
PORT = 5002              # Chef Py-1's station "door number"

ORDER_BOOK_CONFIG = {
    'host':     'database',
    'port':     3306,
    'user':     'itt440_user',
    'password': 'itt440_pass',
    'database': 'itt440_db'
}

CHEF_NAME = 'python_server_user_1'   # Chef Py-1's name in the Order Book


def open_order_book():
    """Connect to the Order Book. Retry every 3s if it's not ready yet."""
    while True:
        try:
            return mysql.connector.connect(**ORDER_BOOK_CONFIG)
        except mysql.connector.Error as e:
            print(f"[Order Book] Not ready yet, retrying in 3s... ({e})")
            time.sleep(3)


def cook_one_dish():
    """Background timer: cooks (and logs) one dish every 30 seconds."""
    while True:
        try:
            conn = open_order_book()
            cursor = conn.cursor()
            cursor.execute("""
                INSERT INTO socket_data (user, points, datetime_stamp)
                VALUES (%s, 1, NOW())
                ON DUPLICATE KEY UPDATE
                    points         = points + 1,
                    datetime_stamp = NOW()
            """, (CHEF_NAME,))
            conn.commit()
            cursor.close()
            conn.close()
            print(f"[{datetime.now()}] 🍳 Chef Py-1 cooked a dish! Logged in Order Book.")
        except Exception as e:
            print(f"[Order Book ERROR] {e}")
        time.sleep(30)


# ── Command 1: GET_POINTS ──────────────────────────────────────
def handle_get_points():
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT user, points, datetime_stamp
        FROM socket_data WHERE user = %s
    """, (CHEF_NAME,))
    row = cursor.fetchone()
    cursor.close()
    conn.close()
    if row:
        return f"Chef: {row[0]} | Dishes Cooked: {row[1]} | Last Dish: {row[2]}"
    return "No record found."


# ── Command 2: GET_HISTORY ─────────────────────────────────────
def handle_get_history():
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT points_before, points_after, updated_at
        FROM socket_data_history
        WHERE user = %s
        ORDER BY updated_at DESC
        LIMIT 5
    """, (CHEF_NAME,))
    rows = cursor.fetchall()
    cursor.close()
    conn.close()

    if not rows:
        return "Kitchen Activity Log is empty so far - no dishes logged yet."

    lines = ["Kitchen Activity Log (last 5 dishes):"]
    for before, after, ts in rows:
        lines.append(f"  {before} -> {after} dishes @ {ts}")
    return "\n".join(lines)


# ── Command 3: GET_RANK ────────────────────────────────────────
def handle_get_rank():
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT `rank`, points
        FROM leaderboard
        WHERE user = %s
    """, (CHEF_NAME,))
    row = cursor.fetchone()
    cursor.close()
    conn.close()
    if row:
        return f"Chef Py-1's Rank: #{row[0]} on the Top Chef Board (with {row[1]} dishes)"
    return "Not ranked yet - no dishes cooked."


# ── Command 4: GET_TIME ────────────────────────────────────────
def handle_get_time():
    conn = open_order_book()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT datetime_stamp
        FROM socket_data WHERE user = %s
    """, (CHEF_NAME,))
    row = cursor.fetchone()
    cursor.close()
    conn.close()
    if row:
        return f"Chef Py-1's last dish was cooked at: {row[0]}"
    return "No record found."


# ── Command Router ──────────────────────────────────────────────
COMMANDS = {
    "GET_POINTS":  handle_get_points,
    "GET_HISTORY": handle_get_history,
    "GET_RANK":    handle_get_rank,
    "GET_TIME":    handle_get_time,
}


def serve_waiter(conn, addr):
    """Handle one Waiter's questions at the counter."""
    print(f"[+] A Waiter approached the counter: {addr}")
    try:
        while True:
            data = conn.recv(1024).decode('utf-8').strip()
            if not data:
                break

            command = data.upper()
            print(f"[Waiter asked] {command}")

            if command in COMMANDS:
                response = COMMANDS[command]()
            else:
                response = (
                    "Chef Py-1 doesn't understand. Try one of:\n"
                    "  GET_POINTS, GET_HISTORY, GET_RANK, GET_TIME"
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
    print(f"[Chef Py-1] 👨‍🍳 Kitchen station open on port {PORT}. Ready to cook!")
    print(f"[Chef Py-1] Accepted commands: {', '.join(COMMANDS.keys())}")

    while True:
        conn, addr = counter.accept()
        threading.Thread(target=serve_waiter, args=(conn, addr), daemon=True).start()


if __name__ == '__main__':
    main()
