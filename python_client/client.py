"""
=================================================================
 THE DOCKER DINER - Waiter for Chef Py-1 (CLIENT)
=================================================================
 ELI5: This Waiter can now ask Chef Py-1 FOUR different
 questions, not just one:

   GET_POINTS  -> "How many dishes have you cooked?"
   GET_HISTORY -> "Show me your last 5 dish updates."
   GET_RANK    -> "What's your rank on the Top Chef Board?"
   GET_TIME    -> "When did you cook your last dish?"

 The Waiter ONLY talks to Chef Py-1 - never to Chef C or Chef Py-2.
=================================================================
"""

import socket
import time

CHEF_PY1_STATION = 'python_server1'   # Chef Py-1's station name on the network
CHEF_PY1_PORT = 5002

MENU = """
========================================
  THE DOCKER DINER - Waiter for Chef Py-1
========================================
  Pick a question to ask Chef Py-1:

  1) GET_POINTS  - How many dishes cooked?
  2) GET_HISTORY - Last 5 dish updates
  3) GET_RANK    - My rank on Top Chef Board
  4) GET_TIME    - When was the last dish?

  Type 'exit' to leave the counter
========================================
"""


def approach_counter():
    """Walk up to Chef Py-1's counter. Wait and retry if it's not open yet."""
    while True:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((CHEF_PY1_STATION, CHEF_PY1_PORT))
            print(f"[Waiter] Reached Chef Py-1's counter ({CHEF_PY1_STATION}:{CHEF_PY1_PORT})")
            return s
        except Exception as e:
            print(f"[Waiter] Chef Py-1's counter not open yet, waiting 3s... ({e})")
            time.sleep(3)


def main():
    sock = approach_counter()
    print(MENU)

    while True:
        try:
            question = input("Ask Chef Py-1: ").strip()
            if not question:
                continue
            if question.lower() == 'exit':
                print("Waiter leaves the counter. Goodbye!")
                break

            sock.sendall((question + "\n").encode('utf-8'))
            answer = sock.recv(1024).decode('utf-8').strip()
            print(f"\n[Chef Py-1 says]\n{answer}\n")

        except (BrokenPipeError, ConnectionResetError):
            print("[Waiter] Lost connection. Walking back to the counter...")
            sock = approach_counter()
        except KeyboardInterrupt:
            print("\nLeaving...")
            break

    sock.close()


if __name__ == '__main__':
    main()
