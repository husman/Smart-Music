package com.smartmusic.udpconnection;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
 
import android.content.Context;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.net.DhcpInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.util.Log;
import android.widget.TextView;
 
public class Client implements Runnable {
	private TextView txt1;
	private byte[] receiveData;
	private Handler hdl;
	
	private UsbDeviceConnection connection;
	private UsbEndpoint endpoint;
	private static int TIMEOUT = 0;
	
	public Client(TextView txt, UsbDeviceConnection conn, UsbEndpoint endp) {
		hdl = new Handler();
		txt1 = txt;
		connection = conn;
		endpoint = endp;
		receiveData = new byte[1024];
	}
    @Override
    public void run() {
            try {
                    // Retrieve the ServerName
                    InetAddress serverAddr = InetAddress.getByName("67.85.190.87");
                   
                    Log.d("UDP", "C: Connecting...");
                    /* Create new UDP-Socket */
                    DatagramSocket socket = new DatagramSocket();
                   
                    /* Prepare some data to be sent. */
                    byte[] buf = ("START\0").getBytes();
                   
                    /* Create UDP-packet with
                     * data & destination(url+port) */
                    DatagramPacket packet = new DatagramPacket(buf, buf.length, serverAddr, 4444);
                    Log.d("UDP", "C: Sending: '" + new String(buf) + "'");
                   
                    /* Send out the packet */
                    socket.send(packet);
                    Log.d("UDP", "C: Sent.");
                    
                    
                    while(true) {
	                    DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
	                    socket.receive(receivePacket);
	                    final byte[] bytes = receivePacket.getData();
	                    
	                    if(hdl != null) {
		                    hdl.post(new Runnable() {
								
								@Override
								public void run() {
									//connection.bulkTransfer(endpoint, bytes, 4, TIMEOUT);
									 txt1.append("[FROM SERVER]: " 
					                    		+ String.format("%02x",bytes[0]) + String.format(" %02x",bytes[1])
					                    		+ String.format(" %02x",bytes[2]) + String.format(" %02x",bytes[3])
					                    		+ "\n");
								}
							});
	                    }
	                    
	                    if(bytes[0] == (byte)0x09 && bytes[1] == (byte)0x90) {
	                    	Log.d("UDP", "[FROMT SERVER]: 09 90 " + String.format("%02x",bytes[2])
	                    			+ String.format(" %02x",bytes[2]));
	                		connection.bulkTransfer(endpoint, bytes, 4, TIMEOUT);
	                    } else if(bytes[0] == (byte)0x08 && bytes[1] == (byte)0x80) {
	                    	Log.d("UDP", "[FROMT SERVER]: 08 80" + String.format("%02x",bytes[2])
	                    			+ String.format(" %02x",bytes[2]));
	                    	connection.bulkTransfer(endpoint, bytes, 4, TIMEOUT);
	                    } else if(bytes[0] == (byte)0x0B && bytes[1] == (byte)0xB0) {
	                    	Log.d("UDP", "[FROMT SERVER]: 0b b0" + String.format("%02x",bytes[2])
	                    			+ String.format(" %02x",bytes[2]));
	                    	connection.bulkTransfer(endpoint, bytes, 4, TIMEOUT);
	                    } else if(bytes[0] == (byte)0x0C && bytes[1] == (byte)0xC0) {
	                    	Log.d("UDP", "[FROMT SERVER]: 0c c0" + String.format("%02x",bytes[2])
	                    			+ String.format(" %02x",bytes[2]));
	                    	connection.bulkTransfer(endpoint, bytes, 4, TIMEOUT);
	                    }
	                    
	                    if(bytes[0] == 0xFF && bytes[1] == 0xFF
	                    	&& bytes[2] == 0xFF && bytes[3] == 0xFF)
	                    	break;
                    }
                    Log.d("UDP", "C: Done.");
            } catch (Exception e) {
                    Log.e("UDP", "C: Error", e);
            }
    }
}

