/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package udpclient;

import java.applet.Applet;
import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.io.*;
import java.net.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.SwingUtilities;

public class UDPClient extends Applet
{
    class SimpleThread extends Thread {
        MidiPlayer mp = null;
        Socket kkSocket = null;
        PrintWriter out = null;
        BufferedReader in = null;
        JTextArea txt = null;
        
        String fromServer;
        String fromUser;
        
        DatagramSocket clientSocket;
        byte[] receiveData;
        int zz = 0;
        InetAddress IPAddress;
        
        public SimpleThread(JTextArea textArea) {
            try {
                txt = textArea;
                mp = new MidiPlayer();
                
                txt.append("[STATUS MSG] Connecting to 67.85.190.87 on port 4444\n\n");
                
                clientSocket = new DatagramSocket();
                IPAddress = InetAddress.getByName("67.85.190.87");
                byte[] sendData = new byte[1024];
                receiveData = new byte[1024];
                String sentence = new String("START\0");
                sendData = sentence.getBytes();
                DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, 4444);
                clientSocket.send(sendPacket);
                txt.append("Sending start packet to server...\n");

            } catch (SocketException ex) {
                txt.append(ex.toString());
            } catch (UnknownHostException ex) {
                txt.append(ex.toString());
            } catch (IOException ex) {
                txt.append(ex.toString());
            }
        }
        public void run() {
            while(true) {
                try {
                    DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
                    clientSocket.receive(receivePacket);
                    String modifiedSentence = new String(receivePacket.getData());
                    txt.append("FROM SERVER:" + modifiedSentence);

                    // Parse command from server
                    if(modifiedSentence.substring(0, 2).equals("09") && 
                    modifiedSentence.substring(3, 5).equals("90")) {
                        mp.play(Integer.parseInt(modifiedSentence.substring(6, 8),16),
                                Integer.parseInt(modifiedSentence.substring(9, 11),16));
                    } else if(modifiedSentence.substring(0, 2).equals("08") && 
                            modifiedSentence.substring(3, 5).equals("80")) {
                        mp.release(Integer.parseInt(modifiedSentence.substring(6, 8),16),
                                Integer.parseInt(modifiedSentence.substring(9, 11),16));
                    } else if(modifiedSentence.substring(0, 2).equals("0c") && 
                            modifiedSentence.substring(3, 5).equals("c0")) {
                            mp.setProgramChange(Integer.parseInt(modifiedSentence.substring(6, 8),16));
                    } else if(modifiedSentence.substring(0, 2).equals("0b") && 
                            modifiedSentence.substring(3, 5).equals("b0")) {
                        mp.setControlChange(Integer.parseInt(modifiedSentence.substring(6, 8),16),
                                            Integer.parseInt(modifiedSentence.substring(9, 11),16));
                    }
                    
                    if(zz++ == 500) {
                        byte[] sendData = new byte[15];
                        String sentence = new String("CONTINUE\0");
                        sendData = sentence.getBytes();
                        DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, 4444);
                        clientSocket.send(sendPacket);
                        txt.append("Sending continue packet to server...\n");
                        zz = 0;
                    }

                    if(modifiedSentence.substring(0, 2).equals("bb") && 
                        modifiedSentence.substring(3, 5).equals("b0") &&
                        modifiedSentence.substring(6, 8).equals("40") &&
                        modifiedSentence.substring(9, 11).equals("00"))
                        break;
                } catch (IOException ex) {
                    txt.append(ex.toString());
                }
            }
            clientSocket.close();
            txt.append("Closed Connection...\n");
        }
    }
    
    public void init() {
        //Execute a job on the event-dispatching thread; creating this applet's GUI.
        try {
            SwingUtilities.invokeAndWait(new Runnable() {
                public void run() {
                    setLayout(new GridLayout(2,1));
                    JLabel lbl = new JLabel("[UDP/IP] Test Prototype: C-server, Java-client, Midi Streaming.");
                    add(lbl);

                    JTextArea textArea = new JTextArea(3, 10);
                    textArea.setEditable(false);
                    JScrollPane scrollPane = new JScrollPane(textArea);
                    add(scrollPane, BorderLayout.CENTER);
                    try {
                        connectToServer(textArea);
                    } catch (IOException ex) {
                        textArea.append(ex.toString());
                    }
                }
            });
        } catch (Exception e) {
            System.err.println("createGUI didn't complete successfully");
        }
        System.out.println("...");
    }
   public void connectToServer(JTextArea txt) throws IOException {
       new SimpleThread(txt).start();
    }
}