package bgu.spl.net.impl.stomp;

import java.util.function.Supplier;
import bgu.spl.net.impl.data.Database;
import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.StompMessagingProtocol; 

import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("There aren't enough args to condact a proper server");
            System.exit(1); 
        }

        int port = 7777;
        try {
            port = Integer.parseInt(args[0]);
        } catch (NumberFormatException e) {
            System.out.println("Invalid port number: " + args[0]);
            System.exit(1);
        }

        String srv_type = args[1];
        Supplier<StompMessagingProtocol<String>> new_SMP = () -> new StompMessagingProtocolIMPL();
        Supplier<MessageEncoderDecoder<String>> new_SED = () -> new StompEncoderDecoder();

        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            Database.getInstance().printReport();     //we got this idea from https://www.quora.com/How-do-I-use-System-exit-in-Java-to-print-a-message-to-the-console-as-well-as-exit-the-system
        }));                                          // when the server closees in any way, it will print the report function from data base

        

        if (srv_type.equalsIgnoreCase("tpc")) {
            System.out.println("Starting TPC server on port: " + port);
            Server.threadPerClient(port, new_SMP, new_SED).serve();
        }

        else if (srv_type.equalsIgnoreCase("reactor")) {
            System.out.println("Starting Reactor server on port: " + port);
            Server.reactor(Runtime.getRuntime().availableProcessors(), port, new_SMP, new_SED).serve();
        }

        else {
            System.out.println("Unknown server type: " + srv_type + ", please try again with TPC/Reactor");
            System.exit(1);
        }
        
        
    }
}