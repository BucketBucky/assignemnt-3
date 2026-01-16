#include "../include/StompProtocol.h"
#include <iostream>
#include <sstream> 
#include "../include/event.h"
#include <fstream>
// חובה בשביל stringstream

StompProtocol::StompProtocol() : numOfSubs(0), numOfReciptes(0), activeSubs() {}


std::vector<std::string> StompProtocol::process(std::string userLine) {
    // יוצרים "זרם" מהשורה שהמשתמש הקליד
    std::lock_guard<std::mutex> guard(lock);
    std::stringstream keyboardInputstream(userLine);
    
    std::string command;
    keyboardInputstream >> command; // שואבים את המילה הראשונה (הפקודה)
    std::vector<std::string> multiFrames;
    // --- טיפול בפקודת LOGIN ---
    if (command == "login") {
        std::string hostPort, username, password;
        
        // שואבים את המילים הבאות לפי הסדר
        if (!(keyboardInputstream >> hostPort >> username >> password)) {
            // אם לא הצלחנו לקרוא את כל 3 המילים, סימן שהמשתמש לא הקליד מספיק
            std::cout << "hostport username or password is missing" << std::endl;
            return multiFrames;
        }
        whosent=username;
        
        // בניית ההודעה בפורמט STOMP
        // שים לב: אנחנו בונים סטרינג אחד ארוך עם ירידות שורה
        std::string frame = "CONNECT\n";
        frame = frame + "accept-version:1.2\n";
        frame = frame + "host:stomp.cs.bgu.ac.il\n";
        frame =frame + "login:" + username + "\n";
        frame = frame + "passcode:" + password + "\n";
        frame = frame + "\n"; // שורה ריקה לסיום הכותרות
        multiFrames.push_back(frame);
        return multiFrames;
    }

    // --- JOIN (Subscribe) ---
    if (command == "join") {
        std::string gameName;
        if (!(keyboardInputstream >> gameName)) { // אם חסר שם המשחק
            std::cout << "Usage: join {game_name}" << std::endl;
            return multiFrames;
        }

        // בדיקה: האם כבר נרשמנו למשחק הזה?
        // count מחזיר 1 אם קיים ו-0 אם לא
        if (activeSubs.count(gameName)) {
            std::cout << "Already subscribed to " << gameName << std::endl;
            return multiFrames;
        }

        // יצירת ID חדש
        numOfSubs++;
        int id = numOfSubs;
        
        // שמירה במילון לשימוש עתידי (כשנרצה לעשות exit)
        activeSubs[gameName] = id;

        // בניית הפריים
        std::string frame = "SUBSCRIBE\n";
        frame = frame + "destination:/" + gameName + "\n"; // לפי הפרוטוקול הנושא מתחיל ב-/
        frame = frame + "id:" + std::to_string(id) + "\n";
        frame = frame + "receipt:" + std::to_string(id) + "\n"; // נבקש אישור מהשרת
        frame = frame + "\n";

        multiFrames.push_back(frame);
        return multiFrames;
    }

    // --- 3. EXIT (Unsubscribe) ---
    if (command == "exit") {
        std::string gameName;
        if (!(keyboardInputstream >> gameName)) {
            std::cout << "no name" << std::endl;
            return multiFrames;
        }

        // חיפוש במילון: האם אנחנו רשומים למשחק הזה?
        // iterator הוא כמו מצביע לאיבר במילון
        auto subPointer = activeSubs.find(gameName);
        
        if (subPointer == activeSubs.end()) {
            std::cout << "Not subscribed to " << gameName << std::endl;
            return multiFrames;
        }

        // שליפת ה-ID מתוך האיבר שמצאנו (it->second זה הערך, כלומר ה-ID)
        int id = subPointer->second;
        
        // מחיקה מהמילון - כי אנחנו מתנתקים
        activeSubs.erase(subPointer);

        std::string frame = "UNSUBSCRIBE\n";
        frame = frame + "id:" + std::to_string(id) + "\n";
        frame = frame + "receipt:" + std::to_string(id) + "\n"; 
        frame = frame + "\n";
        multiFrames.push_back(frame);
        return multiFrames;
    }
    // --- 4. LOGOUT (Disconnect) ---
    if (command == "logout") {
        // ב-Disconnect נהוג לשלוח receipt כדי לוודא סגירה נקייה
        // נשתמש ב-receiptIdCounter שעדיין לא השתמשנו בו
        numOfReciptes++;
        
        std::string frame = "DISCONNECT\n";
        frame = frame + "receipt:" + std::to_string(numOfReciptes) + "\n";
        frame = frame + "\n";
       multiFrames.push_back(frame);
        return multiFrames;
    }

    // --- 5. REPORT (החלק החדש!) ---
    if (command == "report") {
        std::string jsonFile;
        if (!(keyboardInputstream >> jsonFile)) {
             std::cout << "Usage: report {file_path}" << std::endl;
             return multiFrames;
        }

        // שימוש במחלקה שקיבלת מהסגל לקריאת הקובץ
        
        names_and_events data;
        try {
            data = parseEventsFile(jsonFile);
        } catch (const std::exception& e) {
            std::cout << "Error parsing file: " << e.what() << std::endl;
            return multiFrames;
        }

        // שם הערוץ הוא שרשור שמות הקבוצות
        std::string gameName = data.team_a_name + "_" + data.team_b_name;

        // עוברים על כל האירועים בקובץ
        for (const Event& event : data.events) {
            std::string frame = "SEND\n";
            frame = frame + "destination:/" + gameName + "\n";
            frame = frame + "\n"; // סיום כותרות

            // גוף ההודעה לפי הפורמט הנדרש
            frame = frame + "user: " + whosent + "\n";
            frame = frame + "team a: " + data.team_a_name + "\n";
            frame = frame + "team b: " + data.team_b_name + "\n";
            frame = frame + "event name: " + event.get_name() + "\n";
            frame = frame + "time: " + std::to_string(event.get_time()) + "\n";
            
            frame = frame + "general game updates:\n";
            for (const auto& pair : event.get_game_updates()) {
                frame = frame + pair.first + ":" + pair.second + "\n";
            }
            
            frame = frame + "team a updates:\n";
            for (const auto& pair : event.get_team_a_updates()) {
                frame = frame + pair.first + ":" + pair.second + "\n";
            }
            
            frame = frame + "team b updates:\n";
            for (const auto& pair : event.get_team_b_updates()) {
                frame = frame + pair.first + ":" + pair.second + "\n";
            }
            
            frame = frame + "description:\n" + event.get_discription() + "\n";
            
            // הוספת הפריים לרשימה
            multiFrames.push_back(frame);
        }
        
        std::cout << "Generated " << multiFrames.size() << " frames from report." << std::endl;
        return multiFrames;
    }
    //summary
    else if (command == "summary") {
        std::string gameName, user, file;
        // מנסים לקרוא 3 מילים מהקלט: שם משחק, שם משתמש, נתיב קובץ
        if (!(keyboardInputstream >> gameName >> user >> file)) {
            std::cout << "Usage: summary {game_name} {user} {file}" << std::endl;
            return multiFrames;
        }

        // 1. בדיקה האם יש לנו בכלל מידע על המשחק הזה
        if (AllGamesInfo.find(gameName) == AllGamesInfo.end()) {
            std::cout << "Error: No data exists for game " << gameName << std::endl;
            return multiFrames;
        }

        // שליפת הנתונים מהזיכרון למשתנה נוח
        const GameMemory& game = AllGamesInfo[gameName];

        // 2. פתיחת הקובץ לכתיבה
        // (הפרמטר std::ios::trunc אומר: אם הקובץ קיים, תמחק את התוכן שלו ותתחיל מחדש)
        std::ofstream summaryFile(file, std::ios::trunc);
        
        if (!summaryFile.is_open()) {
            std::cout << "Error: Could not open file " << file << std::endl;
            return multiFrames;
        }

        // 3. כתיבת הסטטיסטיקות (החלק הראשון בקובץ)
        summaryFile << game.team_a << " vs " << game.team_b << "\n";
        summaryFile << "Game stats:\n";
        
        summaryFile << "General stats:\n";
        for (const auto& pair : game.general_stats) {
            summaryFile << pair.first << ": " << pair.second << "\n";
        }

        summaryFile << game.team_a << " stats:\n";
        for (const auto& pair : game.team_a_stats) {
            summaryFile << pair.first << ": " << pair.second << "\n";
        }

        summaryFile << game.team_b << " stats:\n";
        for (const auto& pair : game.team_b_stats) {
            summaryFile << pair.first << ": " << pair.second << "\n";
        }

        // 4. כתיבת היסטוריית האירועים (החלק השני בקובץ)
        summaryFile << "Game event reports:\n";
        
        // עוברים על כל האירועים שנשמרו בוקטור
        for (const Event& event : game.events) {
            // פורמט: זמן - שם אירוע
            summaryFile << event.get_time() << " - " << event.get_name() << ":\n\n";
            
            // תיאור האירוע (עד 300 תווים, לפי המטלה)
            summaryFile << event.get_discription() << "\n\n\n";
        }

        summaryFile.close();
        std::cout << "Summary created successfully: " << file << std::endl;
        
        // פקודת summary היא פנימית של הלקוח, לא שולחים כלום לשרת
        return multiFrames; 
    }

    // אם הפקודה לא מוכרת
    std::cout << "Unknown command" << std::endl;
    return multiFrames;;
}



void StompProtocol::processAnswer(std::string serverResponse) {
   std::lock_guard<std::mutex> guard(lock);
    
    std::stringstream socketInput(serverResponse);
    std::string line;
    std::string command;
    std::getline(socketInput, command); // קריאת סוג הפקודה (CONNECTED, MESSAGE...)

    // --- טיפול בהודעת MESSAGE (עדכון משחק) ---
    if (command == "MESSAGE") {
        std::string destination;
        std::string user;
        std::string team_a;
        std::string team_b;
        std::string event_name;
        int time = 0;
        std::string description;
        
        // מפות זמניות לעדכון
        std::map<std::string, std::string> general_updates;
        std::map<std::string, std::string> a_updates;
        std::map<std::string, std::string> b_updates;

        // 1. קריאת הכותרות (Headers)
        while (std::getline(socketInput, line) && line != "") {
            if (line.find("destination:") == 0) {
                destination = line.substr(12); // דילוג על "destination:"
            }
        }
        // הסרת ה-/ משם המשחק (למשל /Germany_Japan -> Germany_Japan)
        std::string gameName = destination.substr(1);

        // 2. קריאת הגוף (Body) ופירוק המידע
        // הגוף בנוי בצורה מאוד ספציפית, נקרא אותו שורה שורה
        std::string currentSection = "";
        
        while (std::getline(socketInput, line)) {
            if (line.find("user:") == 0) user = line.substr(6);
            else if (line.find("team a:") == 0) team_a = line.substr(8);
            else if (line.find("team b:") == 0) team_b = line.substr(8);
            else if (line.find("event name:") == 0) event_name = line.substr(12);
            else if (line.find("time:") == 0) time = std::stoi(line.substr(6));
            else if (line == "general game updates:") currentSection = "general";
            else if (line == "team a updates:") currentSection = "team_a";
            else if (line == "team b updates:") currentSection = "team_b";
            else if (line == "description:") currentSection = "description";
            else {
                // אנחנו בתוך סקציה, צריך לעבד את המידע
                if (currentSection == "description") {
                    description += line + "\n";
                }
                else if (line.find(":") != std::string::npos) {
                    // זה עדכון סטטיסטיקה (key:value)
                    int split = line.find(":");
                    std::string key = line.substr(0, split);
                    std::string val = line.substr(split + 1);
                    
                    if (currentSection == "general") general_updates[key] = val;
                    if (currentSection == "team_a") a_updates[key] = val;
                    if (currentSection == "team_b") b_updates[key] = val;
                }
            }
        }

        // 3. עדכון הזיכרון (המפה gamesData)
        
        // אם המשחק לא קיים עדיין בזיכרון, ניצור אותו
        if (AllGamesInfo.find(gameName) == AllGamesInfo.end()) {
            AllGamesInfo[gameName].team_a = team_a;
            AllGamesInfo[gameName].team_b = team_b;
        }

        // הוספת האירוע לרשימת האירועים
        Event event(team_a, team_b, event_name, time, general_updates, a_updates, b_updates, description);
        AllGamesInfo[gameName].events.push_back(event);

        // עדכון הסטטיסטיקות המצטברות (דורס את הישן)
        for (auto const& [key, val] : general_updates) AllGamesInfo[gameName].general_stats[key] = val;
        for (auto const& [key, val] : a_updates) AllGamesInfo[gameName].team_a_stats[key] = val;
        for (auto const& [key, val] : b_updates) AllGamesInfo[gameName].team_b_stats[key] = val;

        std::cout << "Updated game stats for: " << gameName << std::endl;
    }

    // --- טיפול בשאר ההודעות (כמו קודם) ---
    else if (command == "CONNECTED") {
        std::cout << "Login successful" << std::endl;
    }
    else if (command == "ERROR") {
        std::cout << "Error:\n" << serverResponse << std::endl;
    }
    else if (command == "RECEIPT") {
        std::cout << "Receipt received" << std::endl;
    }
}