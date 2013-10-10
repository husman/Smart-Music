/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package udpclient;

import java.applet.Applet;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.Toolkit;
import java.io.*;
import java.net.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.sound.midi.Instrument;
import javax.sound.midi.MidiSystem;
import javax.sound.midi.MidiUnavailableException;
import javax.sound.midi.Synthesizer;
import javax.swing.*;

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
        while (true) {
            try {
                DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
                clientSocket.receive(receivePacket);
                String modifiedSentence = new String(receivePacket.getData());
                byte[] bytes = receivePacket.getData();
                
                // Parse command from server
                if(bytes[0] == (byte)0x09 && bytes[1] == (byte)0x90) {
                        mp.play(bytes[2],bytes[3]);
                    } else if(bytes[0] == (byte)0x08 && bytes[1] == (byte)0x80) {
                        mp.release(bytes[2],bytes[3]);
                    } else if(bytes[0] == (byte)0x0C && bytes[1] == (byte)0xC0) {
                        mp.setProgramChange(bytes[2]);
                    } else if(bytes[0] == (byte)0x0B && bytes[1] == (byte)0xB0) {
                        mp.setControlChange(bytes[2],bytes[3]);
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

                    if(bytes[0] == (byte)0x0B && bytes[1] == (byte)0xB0
                       && bytes[2] == (byte)0x40 && bytes[3] == (byte)0x00)
                        break;
                    
                    txt.append("FROM SERVER:" + String.format(" %02x", bytes[0])
                                + String.format(" %02x", bytes[1])
                                + String.format(" %02x", bytes[2])
                                + String.format(" %02x", bytes[3])+"\n");
            } catch (IOException ex) {
                txt.append(ex.toString());
            }
        }
        clientSocket.close();
        txt.append("Closed Connection...\n");
        
    }
}

public class UDPClient extends  Applet {

    private static JTextArea txt;

    public void init() {
        //Execute a job on the event-dispatching thread; creating this applet's GUI.
        try {
            SwingUtilities.invokeAndWait(new Runnable() {

                public void run() {
                    try {
                        final Toolkit toolkit = Toolkit.getDefaultToolkit();
                        final Dimension screenSize = toolkit.getScreenSize();

                        setLayout(new GridLayout(1, 1));
                        JLabel lbl = new JLabel("[UDP/IP] Test Prototype: C-server, Java-client, Midi Streaming.");
                        add(lbl, BorderLayout.NORTH);
                        
                        //JFrame.setde

                        txt = new JTextArea(3, 5);
                        txt.setEditable(false);
                        JScrollPane scrollPane = new JScrollPane(txt);
                        add(scrollPane, BorderLayout.CENTER);

                        Synthesizer synthesizer = MidiSystem.getSynthesizer();
                        synthesizer.open();
                        Instrument[] orchestra = synthesizer.getAvailableInstruments();

                        StringBuilder sb = new StringBuilder();
                        String eol = System.getProperty("line.separator");
                        sb.append(
                                "The orchestra has "
                                + orchestra.length
                                + " instruments."
                                + eol);
                        for (Instrument instrument : orchestra) {
                            sb.append(instrument.toString());
                            sb.append(eol);
                        }
                        synthesizer.close();
                        JOptionPane.showMessageDialog(null,
                                new JScrollPane(new JTextArea(sb.toString(), 20, 30)));

                        txt.setCaretPosition(txt.getDocument().getLength());
                        
                        try {
                            connectToServer();
                        } catch (IOException ex) {
                            txt.append(ex.toString());
                        }
                    } catch (MidiUnavailableException ex) {
                        Logger.getLogger(UDPClient.class.getName()).log(Level.SEVERE, null, ex);
                    }
                }
            });
        } catch (Exception e) {
            System.err.println("createGUI didn't complete successfully");
        }
        System.out.println("...");
    }

    public static void connectToServer() throws IOException {
        new SimpleThread(txt).start();

    }
}