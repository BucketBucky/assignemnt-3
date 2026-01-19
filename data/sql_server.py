#!/usr/bin/env python3
"""
Basic Python Server for STOMP Assignment – Stage 3.3

IMPORTANT:
DO NOT CHANGE the server name or the basic protocol.
Students should EXTEND this server by implementing
the methods below.
"""
import sqlite3
import socket
import sys
import threading


SERVER_NAME = "STOMP_PYTHON_SQL_SERVER"  # DO NOT CHANGE!
DB_FILE = "stomp_server.db"              # DO NOT CHANGE!




def recv_null_terminated(sock: socket.socket) -> str:
    data = b""
    while True:
        chunk = sock.recv(1024)
        if not chunk:
            return ""
        data += chunk
        if b"\0" in data:
            msg, _ = data.split(b"\0", 1)
            return msg.decode("utf-8", errors="replace")


def init_database(): #we create tables to save the data we want from the server (login times, lougout times, file tracking etc.. )
    dbcon = sqlite3.connect(DB_FILE)
    with dbcon:
        cursor=dbcon.cursor()
        cursor.execute('''CREATE TABLE IF NOT EXISTS users 
                 (username TEXT PRIMARY KEY, password TEXT, registration_date TEXT)''')
        cursor.execute('''CREATE TABLE IF NOT EXISTS login_history 
                 (username TEXT, login_time TEXT, logout_time TEXT)''')
        cursor.execute('''CREATE TABLE IF NOT EXISTS file_tracking 
                 (username TEXT, filename TEXT, upload_time TEXT, game_channel TEXT)''')
        print(f"[{SERVER_NAME}] Database initialized successfully.")
        dbcon.commit()
    dbcon.close()
    


def execute_sql_command(sql_command: str) -> str: #this function apllies changes to the tables we have initialized. (commands like INSERT)
    dbcon = sqlite3.connect(DB_FILE)
    with dbcon:
        cursor=dbcon.cursor()
        cursor.execute(sql_command)
        dbcon.commit()
    dbcon.close()

    return "done"


def execute_sql_query(sql_query: str) -> str: #this function reads data from the tables (select) 
    dbcon = sqlite3.connect(DB_FILE)
    with dbcon:
        cursor=dbcon.cursor()
        cursor.execute(sql_query)
        infoData = cursor.fetchall() #to suck all the data from the selected table
        if not infoData:
            return ""
        infoList = []
        for info in infoData:
            infoString = ", ".join(str(item) for item in info)
            infoList.append(infoString)
    
    dbcon.close()

    return "SUCCESS|" + "|".join(infoList)


def handle_client(client_socket: socket.socket, addr):
    print(f"[{SERVER_NAME}] Client connected from {addr}")

    try:
        while True:
            message = recv_null_terminated(client_socket)
            if message == "":
                break

            message_clean = message.strip()
            response = ""


            print(f"[{SERVER_NAME}] Received: {message}") 
            #we check the type of the command
            if message_clean.upper().startswith("SELECT"):
                response = execute_sql_query(message_clean)
            else:
                response = execute_sql_command(message_clean)
            # we send a null char to close
            client_socket.sendall((response + "\0").encode("utf-8"))

    except Exception as e:
        print(f"[{SERVER_NAME}] Error handling client {addr}: {e}")
    finally:
        try:
            client_socket.close()
        except Exception:
            pass
        print(f"[{SERVER_NAME}] Client {addr} disconnected")


def start_server(host="127.0.0.1", port=7778):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind((host, port))
        server_socket.listen(5)
        print(f"[{SERVER_NAME}] Server started on {host}:{port}")
        print(f"[{SERVER_NAME}] Waiting for connections...")

        while True:
            client_socket, addr = server_socket.accept()
            t = threading.Thread(
                target=handle_client,
                args=(client_socket, addr),
                daemon=True
            )
            t.start()

    except KeyboardInterrupt:
        print(f"\n[{SERVER_NAME}] Shutting down server...")
    finally:
        try:
            server_socket.close()
        except Exception:
            pass


if __name__ == "__main__":
    port = 7778
    init_database() #create the tables 
    if len(sys.argv) > 1:
        raw_port = sys.argv[1].strip()
        try:
            port = int(raw_port)
        except ValueError:
            print(f"Invalid port '{raw_port}', falling back to default {port}")

    start_server(port=port)
