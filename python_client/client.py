"""
The Docker Diner - Waiter client for Chef Py-1.

This client connects to the Python server and lets the waiter
check stats or award extra points.
"""

import socket
import time

CHEF_PY1_STATION = 'python_server1'   # Chef Py-1's station name on the network
CHEF_PY1_PORT = 5002


def build_menu(chef_name):
    return f"""
╔══════════════════════════════════════════╗
║  🍽️  THE DOCKER DINER - Waiter for {chef_name}  ║
╚══════════════════════════════════════════╝
  Pick a question to ask {chef_name}:

  1) 🍳 Chef, how many dishes have you cooked so far?
  2) 🧾 Can you show me the updates for the last 5 dishes?
  3) 🏆 Where do you currently stand on the Top Chef Board?
  4) ⏰ What time did the last dish go out?
  5) 🎁 Waiter's treat! I'd like to award extra dish points to this chef.

  Type 'exit' to leave the counter
========================================
"""


def approach_counter():
    """Walk up to Chef Py-1's counter. Wait and retry if it's not open yet."""
    while True:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((CHEF_PY1_STATION, CHEF_PY1_PORT))
            print(f"✨ [Waiter] Reached Chef Py-1's counter ({CHEF_PY1_STATION}:{CHEF_PY1_PORT})")
            return s
        except Exception as e:
            print(f"⏳ [Waiter] Chef Py-1's counter not open yet, waiting 3s... ({e})")
            time.sleep(3)


def get_chef_name(sock):
    try:
        sock.sendall(b"GET_NAME\n")
        name = sock.recv(1024).decode('utf-8').strip()
        return name or "Chef"
    except Exception:
        return "Chef"


def main():
    sock = approach_counter()
    chef_name = get_chef_name(sock)
    print(f"\n✨ Connected to {chef_name}'s kitchen counter!")
    print(build_menu(chef_name))

    while True:
        try:
            question = input(f"🧑‍🍳 Ask {chef_name} (Type 'exit' to leave the counter): ").strip()
            if not question:
                continue
            if question.lower() == 'exit':
                print("👋 Waiter leaves the counter. Goodbye!")
                break

            if question == '5':
                extra_points = input("🎁 How many extra dish points to add? ").strip()
                if not extra_points.isdigit() or int(extra_points) <= 0:
                    print("⚠️ Please enter a positive number.")
                    continue
                question = f"ADD_POINTS {extra_points}"

            sock.sendall((question + "\n").encode('utf-8'))
            answer = sock.recv(1024).decode('utf-8').strip()
            print(f"\n[{chef_name} says]\n{answer}\n")

        except (BrokenPipeError, ConnectionResetError):
            print("[Waiter] Lost connection. Walking back to the counter...")
            sock = approach_counter()
        except KeyboardInterrupt:
            print("\nLeaving...")
            break

    sock.close()


if __name__ == '__main__':
    main()
