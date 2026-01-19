import subprocess
import time
import os
import sqlite3
import json
import sys

# --- הגדרות (וודא שהנתיבים נכונים אצלך) ---
CLIENT_PATH = "./bin/StompWCIClient"  # או ./StompWCIClient תלוי ב-Makefile
HOST = "127.0.0.1"
PORT = "7777"
DB_PATH = "../data/stomp_server.db"

# צבעים לטרמינל
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
RESET = '\033[0m'

def create_dummy_json():
    """יוצר קובץ JSON דמה לצורך בדיקות הדיווח"""
    data = {
        "team a": "TestA",
        "team b": "TestB",
        "events": [
            {
                "event name": "Goal",
                "time": 10,
                "general game updates": {"active": "true"},
                "team a updates": {"goals": "1"},
                "team b updates": {"goals": "0"},
                "description": "Goal for A"
            }
        ]
    }
    if not os.path.exists("data"):
        os.makedirs("data")
    with open("data/test_event.json", "w") as f:
        json.dump(data, f)
    print(f"{YELLOW}[INFO]{RESET} Created dummy JSON for testing.")

def run_test(name, commands, expected_output_fragment=None, check_db_sql=None):
    print(f"\n--- Running Test: {name} ---")
    
    # הפעלת הלקוח
    try:
        process = subprocess.Popen(
            [CLIENT_PATH, HOST, PORT],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
    except FileNotFoundError:
        print(f"{RED}[ERROR]{RESET} Could not find client executable at: {CLIENT_PATH}")
        return False

    # הכנת הפקודות לשליחה
    full_input = "\n".join(commands) + "\n"
    
    stdout = ""
    stderr = ""
    
    try:
        # שליחת הפקודות והמתנה של מקסימום 2 שניות
        # זה הפתרון לתקיעה!
        stdout, stderr = process.communicate(input=full_input, timeout=2)
    except subprocess.TimeoutExpired:
        # אם הלקוח לא נסגר לבד, אנחנו הורגים אותו וקוראים את הפלט
        process.kill()
        stdout, stderr = process.communicate()
        # זה לא בהכרח כישלון, פשוט הלקוח לא מימש יציאה נקייה
        
    # בדיקה 1: האם הפלט מכיל את מה שציפינו?
    passed = True
    if expected_output_fragment:
        if expected_output_fragment in stdout:
            print(f"{GREEN}[PASS]{RESET} Output contained: '{expected_output_fragment}'")
        else:
            print(f"{RED}[FAIL]{RESET} Expected '{expected_output_fragment}' but got:")
            print("-" * 20)
            print(stdout) # מדפיס את מה שהתקבל בפועל
            print("-" * 20)
            passed = False

    # בדיקה 2: האם הנתונים נשמרו ב-SQL? (רק אם הבדיקה הקודמת עברה)
    if check_db_sql and passed:
        if not os.path.exists(DB_PATH):
             print(f"{RED}[FAIL]{RESET} Database file not found at: {DB_PATH}")
             return False

        try:
            conn = sqlite3.connect(DB_PATH)
            cursor = conn.cursor()
            cursor.execute(check_db_sql)
            result = cursor.fetchone()
            conn.close()
            
            if result:
                print(f"{GREEN}[PASS]{RESET} SQL Check passed: Found {result}")
            else:
                print(f"{RED}[FAIL]{RESET} SQL Check failed. Query returned nothing.")
                passed = False
        except Exception as e:
            print(f"{RED}[FAIL]{RESET} SQL Error: {e}")
            passed = False

    return passed

def main():
    # 1. יצירת קבצים
    create_dummy_json()

    # רשימת המבחנים
    tests = []
    
    # שם משתמש ייחודי לכל הרצה כדי לא להתנגש עם הרצות קודמות ב-DB
    unique_user = f"user_{int(time.time())}"

    # --- מבחן 1: ניסיון דיווח ללא התחברות ---
    tests.append(run_test(
        name="1. Report without Login",
        commands=[
            "report data/test_event.json"
        ],
        expected_output_fragment="You must be logged in"  # מה שהלקוח שלך מדפיס
    ))

    # --- מבחן 2: התחברות תקינה ובדיקת שמירה ב-DB ---
    # זה בודק את התיקון של ה-Java <-> Python
    tests.append(run_test(
        name="2. Login & Persistence Check",
        commands=[
            f"login {HOST}:{PORT} {unique_user} 12345",
            "logout"
        ],
        expected_output_fragment="Login successful", # או CONNECTED
        check_db_sql=f"SELECT username, password FROM users WHERE username='{unique_user}'"
    ))

    # --- מבחן 3: התחברות עם סיסמה שגויה ---
    # זה בודק שהתיקון ב-login עובד ולא יוצר משתמש חדש
    tests.append(run_test(
        name="3. Wrong Password Check",
        commands=[
            f"login {HOST}:{PORT} {unique_user} 999wrong" # סיסמה לא נכונה למשתמש קיים
        ],
        expected_output_fragment="Wrong password" # ההודעה שהשרת מחזיר
    ))

    # --- מבחן 4: הרשמה כפולה לערוץ ---
    tests.append(run_test(
        name="4. Double Join Check",
        commands=[
            f"login {HOST}:{PORT} user_joiner 123",
            "join germany_japan",
            "join germany_japan"
        ],
        expected_output_fragment="already subscribed"
    ))

    # סיכום תוצאות
    print("\n" + "="*30)
    print("TEST SUMMARY")
    print("="*30)
    success_count = sum(tests)
    total = len(tests)
    
    if success_count == total:
        print(f"{GREEN}PERFECT! {success_count}/{total} Tests Passed.{RESET}")
        print(f"{GREEN}The Logic is Solid. 🚀{RESET}")
    else:
        print(f"{RED}WARNING: Only {success_count}/{total} Tests Passed.{RESET}")
        print("Check the logs above to see what failed.")

if __name__ == "__main__":
    main()