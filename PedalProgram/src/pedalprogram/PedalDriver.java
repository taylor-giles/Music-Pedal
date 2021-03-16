package pedalprogram;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import jssc.SerialPort;
import jssc.SerialPortException;
import jssc.SerialPortList;

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author super
 */
public class PedalDriver {
    
    String[] portNames;
    String chosenPort;
    ArrayList<Song> songList = new ArrayList<>();
    final static String LIST_FILENAME = "list.obj";
    final static String SD_CARD_NAME = "Parrot";
    
    final static String RCV_SUFFIX = "\r\n";
    final static String SEND_SUFFIX = "\0";
    
    final static String PING_KEY = "Parrot";
    final static String CONFIRM_KEY = "Confirm";
    final static String CONTINUE_KEY = "Go";
    final static String STANDBY_KEY = "Standby";
    
    final static int SONG_SIZE = 7;
    final static int DIGITS = 4;
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        PedalDriver driver = new PedalDriver();
        
        //Get the list of ports
        driver.portNames = SerialPortList.getPortNames();
        System.out.println("Found: " + Arrays.toString(driver.portNames) + "\n");
        
        //Try each port
        SerialPort port;
        for(String portName : driver.portNames){
            System.out.println("Port: " + portName);
            try {
                //Open port
                port = new SerialPort(portName);
                port.openPort();
                port.setParams(SerialPort.BAUDRATE_115200,
                        SerialPort.DATABITS_8,
                        SerialPort.STOPBITS_1,
                        SerialPort.PARITY_NONE);
                
                //Wait for bootloader
                Thread.sleep(500);
                
                //Listen for ping
                if(gotKey(port, PING_KEY)){
                    System.out.println("Ping received.");
                    
                    //Send confirmation
                    if(sendKey(port, CONFIRM_KEY)){
                        System.out.println("Confirmation sent.");
                    } else {
                        System.out.println("Confirmation not sent");
                    }
                    
                    //Wait
                    Thread.sleep(100);
                    
                    //Listen for confirmation
                    if(gotKey(port, CONFIRM_KEY)){
                        communicate(port);
                        break;
                    }
                }
                
                //Close port if it is not the right one
                port.closePort();
                
            } catch (SerialPortException ex) {
                ex.printStackTrace();
            } catch (InterruptedException ex) {
                ex.printStackTrace();
            }
        }
    }
    
    private static void communicate(SerialPort port){
        System.out.println("\nBegin data transfer.");
        try {
            //Wait
            Thread.sleep(100);
            
            //Get the number of songs
            String numSongsStr = new String(port.readBytes());
            numSongsStr = numSongsStr.replaceAll(RCV_SUFFIX, "");
            int numSongs = Integer.parseInt(numSongsStr);
            System.out.println("Number of songs: " + numSongs);
            
            //Wait
            Thread.sleep(100);
            
            //Tell pedal to continue
            sendKey(port, CONTINUE_KEY);
            
            //Receive song data
            ArrayList<Song> songs = new ArrayList<>();
            for(int i = 0; i < numSongs; i++){
                
                //Wait
                Thread.sleep(1000);
                
                System.out.print(".");
                Song song = new Song();
                try {
                    
                    System.out.println(new String(port.readBytes()).replaceAll(RCV_SUFFIX, ""));
                    //song.setIndex(Integer.parseInt(new String(port.readBytes()).replaceAll(RCV_SUFFIX, "")));
                    //System.out.println(song.getIndex());
                    sendKey(port, CONTINUE_KEY);
                    
                    //Wait
                    Thread.sleep(50);
                    
                    System.out.println(new String(port.readBytes()).replaceAll(RCV_SUFFIX, "").toCharArray());
                    sendKey(port, CONTINUE_KEY);
//                    System.out.println(new String(port.readBytes(DIGITS + RCV_SUFFIX.length())).replaceAll(RCV_SUFFIX, ""));
//                    song.setIndex(Integer.parseInt(new String(port.readBytes(DIGITS + RCV_SUFFIX.length())).replaceAll(RCV_SUFFIX, "")));
//                    song.setNotes(new String(port.readBytes(DIGITS + RCV_SUFFIX.length())).replaceAll(RCV_SUFFIX, "").toCharArray());
//                    songs.add(song);
                } catch (SerialPortException ex) {
                    System.out.println("Error getting song data");
                }

             }
           
            for(Song song : songs){
                System.out.print("Index: " + song.getIndex());
                System.out.println("Notes: " + song.getNotesStr());
            }
            
            //Tell pedal to standby
            sendKey(port, STANDBY_KEY);
            
            port.closePort();
            System.exit(4);
            
            //TODO Collect information from user
            ArrayList<Song> newSongs = new ArrayList<>(getUserData(songs));
            Integer newNumSongs = newSongs.size();
            
            //Contact pedal
            sendKey(port, PING_KEY);
            
            //Wait
            Thread.sleep(100);
                    
            //Listen for continue key
            if(gotKey(port, CONTINUE_KEY)){
                //Send number of songs
                sendKey(port, newNumSongs.toString());
                System.out.println("Number of songs sent.");
               
                //Wait
                Thread.sleep(100);
                
                //Listen for continue key
                if(gotKey(port, CONTINUE_KEY)){
                    //Send new song data
                    //Send ONLY notes data; the order in which songs are sent determines index
                    for(Song newSong : newSongs){
                        sendKey(port, newSong.getNotesStr());
                        Thread.sleep(50);
                    }
                    Thread.sleep(100);
                    System.out.println("New song data sent.");
                }
            }
            port.closePort();
            System.out.println("Port closed.");
            
            
            
        } catch (SerialPortException ex) {
            ex.printStackTrace();
        } catch (InterruptedException ex) {
            Logger.getLogger(PedalDriver.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    
    private static ArrayList<Song> getUserData(ArrayList<Song> songs){
        /* Set the Windows look and feel */
        //<editor-fold defaultstate="collapsed" desc=" Look and feel setting code (optional) ">
        /* If Nimbus (introduced in Java SE 6) is not available, stay with the default look and feel.
         * For details see http://download.oracle.com/javase/tutorial/uiswing/lookandfeel/plaf.html 
         */
        try {
            for (javax.swing.UIManager.LookAndFeelInfo info : javax.swing.UIManager.getInstalledLookAndFeels()) {
                if ("Windows".equals(info.getName())) {
                    javax.swing.UIManager.setLookAndFeel(info.getClassName());
                    break;
                }
            }
        } catch (ClassNotFoundException ex) {
            java.util.logging.Logger.getLogger(PedalGUI.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (InstantiationException ex) {
            java.util.logging.Logger.getLogger(PedalGUI.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (IllegalAccessException ex) {
            java.util.logging.Logger.getLogger(PedalGUI.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (javax.swing.UnsupportedLookAndFeelException ex) {
            java.util.logging.Logger.getLogger(PedalGUI.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        }
        //</editor-fold>

        //Start the GUI
        PedalGUI gui = new PedalGUI(songs);
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                gui.setVisible(true);
            }
        });
        
        //Wait for data to be ready
        while(!gui.isReady()){
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ex) {
                ex.printStackTrace();
            }
        }
        return gui.getData();
    }
    
    private static boolean gotKey(SerialPort port, String key){
        try {
            byte[] buffer = port.readBytes((key + RCV_SUFFIX).length());
            return (Arrays.toString(buffer).equals(Arrays.toString((key + RCV_SUFFIX).getBytes(StandardCharsets.US_ASCII))));
        } catch (SerialPortException ex) {
            return false;
        }
    }
    
    private static boolean sendKey(SerialPort port, String key){
        try {
            port.writeBytes((key + SEND_SUFFIX).getBytes());
            return true;
        } catch (SerialPortException ex) {
            return false;
        }
    }
    
    private static void writeList(ArrayList<Song> songList){
        try {
            FileOutputStream fileStream = new FileOutputStream(LIST_FILENAME);
            ObjectOutputStream objStream = new ObjectOutputStream(fileStream);
            
            objStream.writeObject(songList);
            objStream.close();
            fileStream.close();
            
        } catch (IOException ex) {
            Logger.getLogger(PedalDriver.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    
    private static ArrayList<Song> readList(){
        try {
            FileInputStream fileStream = new FileInputStream(LIST_FILENAME);
            ObjectInputStream objStream = new ObjectInputStream(fileStream); 
              
            ArrayList<Song> obj = (ArrayList<Song>)objStream.readObject(); 
              
            objStream.close(); 
            fileStream.close(); 
            return obj;
            
        } catch(IOException ex) { 
            System.out.println("Existing list not found, creating new."); 
        } catch(ClassNotFoundException ex) { 
            System.out.println("Existing list not found, creating new."); 
        } 
        return new ArrayList<>();
    }
}
