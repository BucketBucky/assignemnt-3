import socket
import threading
import time

HOST = '0.0.0.0'
PORT = 7777

def handle_client(conn, addr):
    print(f"\n[NEW CONNECTION] {addr} connected.")
    msg_id = 1

    try:
        while True:
            data = conn.recv(4096)
            if not data:
                break
            
            raw_msg = data.decode('utf-8')
            frames = raw_msg.split('\0') # יכול להיות שהגיעו כמה הודעות ביחד

            for frame in frames:
                if not frame.strip(): continue
                
                # הדפסה ללוג
                lines = frame.split('\n')
                command = lines[0]
                print(f"[{addr}] Received: {command}")

                # 1. התחברות
                if command == "CONNECT":
                    response = "CONNECTED\nversion:1.2\n\n\0"
                    conn.sendall(response.encode('utf-8'))
                
                # 2. הרשמה (רק מאשרים שקיבלנו)
                elif command == "SUBSCRIBE":
                    print("Client subscribed.")
                    
                # 3. דיווח - כאן הקסם! הופכים SEND ל-MESSAGE ושולחים חזרה
                elif command == "SEND":
                    # אנחנו לוקחים את גוף ההודעה כמו שהוא, ורק משנים את הכותרת
                    # במציאות השרת היה מפרק ומרכיב מחדש, פה נעשה "החלפה" מהירה
                    response = frame.replace("SEND", "MESSAGE", 1)
                    
                    # חייבים להוסיף כותרות שהלקוח מצפה להן
                    # נכניס subscription ו-message-id אחרי ה-destination
                    parts = response.split('\n', 2) 
                    # parts[0] = MESSAGE
                    # parts[1] = destination:/...
                    # parts[2] = rest of body
                    
                    header_injection = f"subscription:0\nmessage-id:{msg_id}\n"
                    final_response = parts[0] + '\n' + parts[1] + '\n' + header_injection + parts[2] + '\0'
                    
                    conn.sendall(final_response.encode('utf-8'))
                    msg_id += 1
                    print(f"-> Echoed back as MESSAGE (id: {msg_id})")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        conn.close()
        print(f"[DISCONNECTED] {addr}")

# הרצת השרת
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen()

print(f"--- ECHO SERVER LISTENING ON {PORT} ---")
while True:
    conn, addr = server.accept()
    thread = threading.Thread(target=handle_client, args=(conn, addr))
    thread.start()