package main.java.bgu.spl.net.impl.stomp;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsIMPL;

public class StompMessagingProtocolIMPL implements StompMessagingProtocol<String> {
    private boolean shouldTerminate = false;
    private int connectionId;
    private ConnectionsIMPL<String> connections;
    private String currentUsername = null;
    private Map<String, String> subscriptions = new HashMap<>();  // sub_id -> topic for subsctiption managment
    private static Map<String, String> users = new ConcurrentHashMap<>();  // username -> password
    private static Set<String> loggedInUsers = ConcurrentHashMap.newKeySet();  // Who is logged in by username


    
    @Override
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = (ConnectionsIMPL<String>) connections;
    }
    
    @Override
    //changed to return String because of the interface change (extended MessagingProtocol that has T process(T message)
    public String process(String message){
        String[] lines = ((String) message).split("\n");
        String FrameCommand = lines[0];
        this.check_frame(FrameCommand, lines, message);
        return null;
    }

	/**
     * @return true if the connection should be terminated
     */
    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    
    /////////////////////////////MAIN HELPER FUNCTIONS/////////////////////////////    
    //gets the value of a header from the frame
    private String get_header(String[] lines, String headerName) {
        for (String line : lines) {
            //checks what starts with the wanted header name
            if (line.startsWith(headerName + ":")) {
                //returns the value of the header. the +1 is to skip the ':' char & .trim removes spaces if exist.
                return line.substring(headerName.length() + 1).trim();
            }
        }
        return null;
    }

    //gets the body of the message.
    private String extractBody(String fullMessage) {
        int splitIndex = fullMessage.indexOf("\n\n");
        if (splitIndex != -1) {
            return fullMessage.substring(splitIndex + 2);
        }
        return "";
    }

    //Checks each frame
    private void check_frame(String frame, String[] lines, String msg) {
        if (frame.equals("CONNECT")) {
            h_connect(lines);
        }
        else if (frame.equals("SEND")) {
            String sended_msg = this.extractBody(msg);
            h_send(lines, sended_msg);
        }
        else if (frame.equals("SUBSCRIBE")) {
            h_sub(lines);
        }
        else if (frame.equals("UNSUBSCRIBE")) {
            h_unsub(lines);
        }
        else if (frame.equals("DISCONNECT")) {
            h_disconnect(lines);
        }
        else {
            String error_msg = "Client-id:" + this.connectionId + "\n" + "messgae: You have entered an improper frame, please enter a new one\n\n"+
                        "The message:\n" + "-----\n" + msg;
            this.send_error(error_msg);
        }
    } 


    
    /////////////////////////////MAIN HELPER FUNCTIONS END/////////////////////////////   


    /*
    Handler functions for each Frame
    */
    private void h_connect(String lines[]) {
        //first, let's clean all the inputs.
        String login = get_header(lines, "login");
        String passcode = get_header(lines, "passcode");
        if (login == null || passcode == null) {
            this.send_error("Login or passcode header is missing, please try again");
            return;
        }
        //Now let's check if the user is allready logged in.


        //potential need for sychronized block here.
        if (loggedInUsers.contains(login)) {
            this.send_error("User is allready logged in, please enter a different username");
            return;
        }
        //The user isn't logged in, let's check if he is allready in the datat base.
        if (users.containsKey(login)){
            //user is in the data base, let's check password.
            if (!(users.get(login).equals(passcode))) { //checks if password isn't equal.   
                send_error("User's password isn't correct, please try again");
                return;
            }
        }
        //else means a new user, let's register him.
        else {
            users.put(login, passcode);
        }
        //everything is good, let's log him in.
        currentUsername = login;
        loggedInUsers.add(login);
        String response = "CONNECTED\n" + "version:1.2\n\n";
        connections.send(this.connectionId, response);
    } 
    
    
    private void h_send(String lines[], String msg) {
        String des = get_header(lines, "destination");
        this.connections.send(des, msg);
        String receiptId = get_header(lines, "receipt");
        if (receiptId != null) {
            send_receipt("receipt-id:" + receiptId + "\n\n");
        }
    } 
    
    
    private void h_sub(String lines[]) {
        String des = get_header(lines, "destination");
        String sub_id = get_header(lines, "id");
        this.subscriptions.put(sub_id, des);
        this.connections.subscribe(this.connectionId, des, sub_id);
        String receiptId = get_header(lines, "receipt");
        if (receiptId != null) {
            send_receipt("receipt-id:" + receiptId + "\n\n");
        }
        //need to handle error.
    } 
    
    
    private void h_unsub(String lines[]) {
        String sub_id = get_header(lines, "id");
        if (sub_id != null) {
            String des_for_unsub = this.subscriptions.get(sub_id);
            if ( des_for_unsub != null) 
                this.connections.unsubscribe(this.connectionId, des_for_unsub);
        }
        String receiptId = get_header(lines, "receipt");
        if (receiptId != null) {
            send_receipt("receipt-id:" + receiptId + "\n\n");
        }
    } 
    
    
    private void h_disconnect(String lines[]) {
        String rec = get_header(lines, "receipt");
        if (rec != null) {
            String rec_msg = "receipt-id:" + rec + "\n\n";
            this.send_receipt(rec_msg);
        }
        this.shouldTerminate = true;
        loggedInUsers.remove(this.currentUsername);
        this.connections.disconnect(this.connectionId);
    } 

    
    /*
    Easy error & receipt handler, note that is satrts with the "RECEIPT" command so it is not needed in the msg.
    */
    private void send_receipt(String msg) {
        String rec_msg = "RECEIPT\n" + msg;
        this.connections.send(this.connectionId, rec_msg);
    }

    private void send_error(String msg) {
        String error_msg = "ERROR\n" + "message:" + msg + "\n\n";
        this.connections.send(this.connectionId, error_msg);
    }

}