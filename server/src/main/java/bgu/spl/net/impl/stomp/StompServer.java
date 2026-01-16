package bgu.spl.net.impl.stomp;

import bgu.spl.net.impl.echo.EchoProtocol;
import bgu.spl.net.impl.echo.LineMessageEncoderDecoder;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        // TODO: implement this
        // בודקים אם קיבלנו פורט, אחרת משתמשים ב-7777 כברירת מחדל
        int port = 7777;
        if (args.length > 0) {
            port = Integer.parseInt(args[0]);
        }

        System.out.println("Starting server on port " + port);

        // שימוש זמני ב-EchoProtocol ו-LineMessageEncoderDecoder
        // רק כדי שנוכל לבדוק את החיבור של הקליינט שלנו.
        // בהמשך נחליף את זה ל-StompProtocol ו-StompEncoderDecoder
        Server.threadPerClient(
                port,
                () -> new EchoProtocol(), // יצירת פרוטוקול לכל לקוח
                () -> new LineMessageEncoderDecoder() // יצירת מפענח לכל לקוח
        ).serve();
    }
}
