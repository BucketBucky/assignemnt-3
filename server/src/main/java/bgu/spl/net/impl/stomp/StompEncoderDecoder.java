package main.java.bgu.spl.net.impl.stomp;

import bgu.spl.net.api.MessageEncoderDecoder;

public class StompEncoderDecoder implements MessageEncoderDecoder<String> {
    private static final byte NULL_CHAR = '\0';
    private byte[] bytes = new byte[1 << 10]; //start with 1k
    private int len = 0;

    @Override
    public String decodeNextByte(byte nextByte) {
        if (nextByte == NULL_CHAR) {
            return popString();
        } 
        pushByte(nextByte);
        return null;
    }

    @Override
    public byte[] encode(String message) {
        return (message + NULL_CHAR).getBytes();
    }

    private String popString() {
        String res = new String(bytes, 0, len, java.nio.charset.StandardCharsets.UTF_8);
        len = 0;
        return res;   
    }

    private void pushByte(byte nextByte) {
        if (len >= bytes.length) {
            bytes = java.util.Arrays.copyOf(bytes, len * 2);
        }
        bytes[len] = nextByte;
        len++;
    }
}