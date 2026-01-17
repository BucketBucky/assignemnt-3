package bgu.spl.net.api;

import bgu.spl.net.srv.Connections;


//added the extation for the proper use of stomp server
//than deleted it beacause of Module unswers in FORUM
public interface StompMessagingProtocol<T>{
	/**
	 * Used to initiate the current client protocol with it's personal connection ID and the connections implementation
	**/
    void start(int connectionId, Connections<T> connections);
    
    void process(T message);
	
	/**
     * @return true if the connection should be terminated
     */
    boolean shouldTerminate();
}