package main.java.bgu.spl.net.srv;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Map;

public class ConnectionsIMPL<T> implements Connections<T> {
    private Map<Integer, ConnectionHandler<T>> ch_map = new ConcurrentHashMap<>(); // connectionId -> ConnectionHandler
    private Map<String, Map<Integer, String>> channelSubscribers = new ConcurrentHashMap<>(); // channel -> (connectionId -> subscriptionId) 
    private AtomicInteger counter = new AtomicInteger(0); //for msg_id 

    @Override
    /*
    Sends a message to a specific client.
    Retrieves the ConnectionHandler associated with the given connectionId and sends the message directly to that client.
    Returns true if the client exists and the message was sent, false otherwise.
    */
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> curr_Handler = ch_map.get(connectionId);
        if (curr_Handler != null) {
            curr_Handler.send(msg);
            return true;
        }
        return false;
    }

    /*
    Sends a message to all clients subscribed to a specific channel (topic).
    Retrieves the list of subscribers for the given channel and iterates through them, 
    sending the message to each subscriber using their connection ID.
    */
    @Override
    public void send(String channel, T msg) {
        Map<Integer, String> subs = channelSubscribers.get(channel);
        if (subs != null) {
            String msg_id = "" + this.counter.getAndIncrement();
            for (Map.Entry<Integer, String> entry : subs.entrySet()) {
                Integer ch_id = entry.getKey();
                String sub_id = entry.getValue();
                String personal_msg = "MESSAGE\n" + "subscription:" + sub_id + "\n" + "message-id:" + msg_id + "\n" +
                                         "destination:" + channel + "\n\n" + msg;
                send(ch_id, (T)(personal_msg));
            }
        }
    }
        
    /*
    Disconnects a client completely.
    Removes the client from the active connections map (ch_map).
    Iterates through all active channels and unsubscribes the client from them,
    ensuring no future messages are attempted to be sent to this disconnected client.
    */
    @Override
    public void disconnect(int connectionId) {
        ch_map.remove(connectionId);
        for (Map<Integer, String> subs : channelSubscribers.values()) {
                subs.remove(connectionId);
            }
    }


    /*
    Registers a new physical connection.
    Maps a unique connectionId to its corresponding ConnectionHandler,
    enabling the server to send messages to this specific client.
    */
    public void connect(int connectionId, ConnectionHandler<T> ch) {
        ch_map.put(connectionId, ch);
    }


    /*
    Subscribes a client to a specific topic (channel).
    If the channel does not exist, it creates a new one (thread-safe).
    It then maps the client's connectionId to their specific subscriptionId within that channel.
    */
    public void subscribe(int connectionId, String channelString, String subscriptionId) {
        Map<Integer, String> channel = channelSubscribers.get(channelString);
        if (channel == null) {
            channel = new ConcurrentHashMap<>();
            Map<Integer, String> existingChannel = channelSubscribers.putIfAbsent(channelString, channel);
            if (existingChannel != null)  {
                channel = existingChannel;
            }
        }
        channel.put(connectionId, subscriptionId);
    }

    /*
    Unsubscribes a client from a specific topic.
    Removes the client from the subscriber list of the specified channelString.
    The client remains connected to the server and other channels but will stop receiving messages from this specific topic.
    */
    public void unsubscribe(Integer connectionId, String channelString) {
        Map<Integer, String> channel = channelSubscribers.get(channelString);
        if (channel != null) {
            channel.remove(connectionId);
        }
    }
}