package bgu.spl.net.srv;

//added these 2 imports
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.ConnectionsIMPL;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Supplier;

public abstract class BaseServer<T> implements Server<T> {

    private final int port;
    private final Supplier<StompMessagingProtocol<T>> protocolFactory;
    private final Supplier<MessageEncoderDecoder<T>> encdecFactory;
    private ServerSocket sock;

    //added 2 fields like in Reactor
    private final ConnectionsIMPL<T> connections = new ConnectionsIMPL<>();
    private final AtomicInteger connectionIdCounter = new AtomicInteger(0);

    public BaseServer(
            int port,
            Supplier<StompMessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> encdecFactory) {

        this.port = port;
        this.protocolFactory = protocolFactory;
        this.encdecFactory = encdecFactory;
		this.sock = null;
        
        

    }

    @Override
    public void serve() {

        try (ServerSocket serverSock = new ServerSocket(port)) {
			System.out.println("Server started");

            this.sock = serverSock; //just to be able to close

            while (!Thread.currentThread().isInterrupted()) {

                Socket clientSock = serverSock.accept();
                //added these 3 lines to handle STOMP protocolFactory & connectionId & INIT the STOMP factory
                StompMessagingProtocol<T> stompProtocol = protocolFactory.get();
                int connectionId = connectionIdCounter.getAndIncrement();
                stompProtocol.start(connectionId, connections);

                BlockingConnectionHandler<T> handler = new BlockingConnectionHandler<>(
                        clientSock,
                        encdecFactory.get(),
                        stompProtocol, connectionId, connections);
                connections.connect(connectionId, handler);
                execute(handler);
            }   
        } catch (IOException ex) {
        }

        System.out.println("server closed!!!");
    }

    @Override
    public void close() throws IOException {
		if (sock != null)
			sock.close();
    }

    protected abstract void execute(BlockingConnectionHandler<T>  handler);

}