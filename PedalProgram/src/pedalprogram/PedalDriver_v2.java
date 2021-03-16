package pedalprogram;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
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
public class PedalDriver_v2 {
    
    private static String[] portNames;
    private static String chosenPort;
    private static ArrayList<Song> songList = new ArrayList<>();
    
    private static PedalGUI gui;
    
    private final static JFileChooser fileChooser = new JFileChooser();
    private static String listFilename;
    private static String sdCardLocation;
    
    private final static String RCV_SUFFIX = "\r\n";
    private final static String SEND_SUFFIX = "\0";
    
    private final static String PING_KEY = "Parrot";
    private final static String CONFIRM_KEY = "Confirm";
    
    private final static int SONG_SIZE = 7;
    private final static int DIGITS = 4;
    
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        //Find SD Card
        boolean chosen = false;
        while(!chosen){
            int choice = JOptionPane.showConfirmDialog(null, "Please locate the SD Card", "SD Card", JOptionPane.OK_CANCEL_OPTION);
            if(choice == JOptionPane.YES_OPTION){
                if (fileChooser.showOpenDialog(null) == JFileChooser.APPROVE_OPTION){
                    File file = (fileChooser.getSelectedFile());
                    listFilename = file.getAbsolutePath();
                    sdCardLocation = file.getAbsolutePath().replaceAll("list.obj", "");
                    chosen = true;
                } else {
                    JOptionPane.showMessageDialog(null, "File not chosen", "File Chooser", JOptionPane.PLAIN_MESSAGE);
                    chosen = false;
                }
            } else {
                System.exit(2);
            }
        }
        
        //Read in list
        songList = readList();
        
        //Update filenames
//        for(Song song : songList){
//            song.setFile(new File(sdCardLocation + "MP3\\" + song.getFile().getName()));
//        }
        
        //Get data from user
        boolean done = true;
        do{
            songList = getSongsFromGUI(songList);
            gui.dispose();
            System.out.println("Data retrieved from user.");

            //Make temp directory
            new File(sdCardLocation + ".temp").mkdir();

            for(Song song : songList){
                String filename = "";
                if(song.getFilepath().contains(sdCardLocation)){
                    filename = String.format("%04d", song.getIndex()) + "_" +
                        song.getFilepath().substring(song.getFilepath().indexOf('_') + 1);
                } else {
                    filename = String.format("%04d", song.getIndex()) + "_" +
                        song.getFilepath().substring(song.getFilepath().lastIndexOf('\\') + 1);
                }

                String filePath = sdCardLocation + ".temp\\" + filename;

                try {
                    new File(filePath).createNewFile();
                    Files.copy(new File(song.getFilepath()).toPath(), new File(filePath).toPath(), StandardCopyOption.REPLACE_EXISTING);
                    song.setFilepath(filePath.replaceAll(".temp", "MP3"));
                } catch(IOException ex){
                    JOptionPane.showMessageDialog(null, "Error processing song file: " + filename, "Error", JOptionPane.ERROR_MESSAGE);
                    ex.printStackTrace();
                    done = false;
                }
            } 
        } while(!done);
        
        try {
            //Delete existing MP3 Folder
            Files.walk(new File(sdCardLocation + "MP3").toPath())
                    .sorted(Comparator.reverseOrder())
                    .map(Path::toFile)
                    .forEach(File::delete);
            
            //Rename temp folder to MP3
            new File(sdCardLocation + ".temp").renameTo(new File(sdCardLocation + "MP3"));
        } catch (IOException ex) {
            ex.printStackTrace();
        }
        
        //Save the list on the SD Card
        writeList(songList);
        
        //Find and communicate with pedal
        SerialPort port = findPort();
        while(port == null){
            JOptionPane.showMessageDialog(null, "Please make sure your Parrot Pedal is connected to the computer.", "Connect to Pedal", JOptionPane.PLAIN_MESSAGE);
        }
        if(port != null){
            communicate(port);
        } else {
            System.err.println("Could not locate serial port.");
        }
    }
    
    private static void communicate(SerialPort port){
        System.out.println("\nBeginning data transfer.");
        try { 
            //Wait
            Thread.sleep(100);
                    
            //Send number of songs
            Integer numSongs = songList.size();
            try {
                //The number of '#' characters is the number of songs
                //(i.e. send one '#' for each song)
                StringBuilder bytes = new StringBuilder();
                for(int i = 0; i < numSongs; i++){
                    bytes.append("#");
                }
                port.writeBytes(bytes.toString().getBytes());
                System.out.println("Sent: " + bytes.toString());
            } catch (SerialPortException ex) {
                System.err.println("Error writing to board");
            }
            System.out.println("Number of songs (" + String.format("%03d", songList.size()) + ") sent.");
               
            //Wait
            Thread.sleep(100);
                
            //Listen for confirmation
            if(gotKey(port, CONFIRM_KEY)){
                Thread.sleep(50);
                //Send new song data
                //Send ONLY the notes; the order in which songs are sent determines index
                for(Song newSong : songList){
                    port.writeBytes(newSong.getNotesStr().getBytes());
                    System.out.println("Sent: " + newSong.getNotesStr());
                    Thread.sleep(50);
                }
                Thread.sleep(100);
                System.out.println("New song data sent.");
            } else {
                System.err.println("Error: Pedal unresponsive.");
            }
            if(gotKey(port, CONFIRM_KEY)){
                port.closePort();
                System.out.println("Port closed.");   
            } else {
                System.err.println("Error: Pedal unresponsive.");
            }
        } catch (SerialPortException ex) {
            ex.printStackTrace();
        } catch (InterruptedException ex) {
            ex.printStackTrace();
        }
    }
    
    //Opens GUI and asks user for data
    private static ArrayList<Song> getSongsFromGUI(ArrayList<Song> songs){
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
        gui = new PedalGUI(songs);
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                gui.setVisible(true);
            }
        });
        
        //Wait for data to be ready
        while(!gui.isReady()){System.out.print("");}
        
        return gui.getData();
    }
    
    //Locates the port that the pedal is plugged into by listening to every available port
    private static SerialPort findPort(){
        //Get the list of ports
        portNames = SerialPortList.getPortNames();
        System.out.println("Found: " + Arrays.toString(portNames) + "\n");
        
        //Try each port
        SerialPort port;
        for(String portName : portNames){
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
                    
                    Thread.sleep(100);
                    
                    //Send confirmation
                    if(sendKey(port, CONFIRM_KEY)){
                        System.out.println("Confirmation sent.");
                    } else {
                        System.err.println("Confirmation not sent");
                    }
                    
                    //Wait
                    Thread.sleep(500);
                    
                    //Listen for confirmation
                    if(gotKey(port, CONFIRM_KEY)){
                        System.out.println("Confirmation received, port chosen.");
                        return port;
                    } else {
                        System.err.println("Confirmation not received.");
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
        return null;
    }
    
    //Waits to receive the specified key on the specified port. Returns true if key was received, false otherwise.
    private static boolean gotKey(SerialPort port, String key){
        try {
            byte[] buffer = port.readBytes((key + RCV_SUFFIX).length());
            return (Arrays.toString(buffer).equals(Arrays.toString((key + RCV_SUFFIX).getBytes(StandardCharsets.US_ASCII))));
        } catch (SerialPortException ex) {
            return false;
        }
    }
    
    //Sends the specified String key to the specified port. Returns true if key was successfully sent, false otherwise
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
            FileOutputStream fileStream = new FileOutputStream(listFilename);
            ObjectOutputStream objStream = new ObjectOutputStream(fileStream);
            
            objStream.writeObject(songList);
            objStream.close();
            fileStream.close();
            
        } catch (IOException ex) {
            Logger.getLogger(PedalDriver.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    
    //Reads the list from SD Card
    private static ArrayList<Song> readList(){
        try {
            FileInputStream fileStream = new FileInputStream(listFilename);
            ObjectInputStream objStream = new ObjectInputStream(fileStream); 
              
            ArrayList<Song> obj = (ArrayList<Song>)objStream.readObject();
            System.out.println("List found");
              
            objStream.close(); 
            fileStream.close(); 
            return obj;
            
        } catch(IOException | ClassNotFoundException ex) { 
            System.out.println("Existing list not found, creating new."); 
        } 
        return new ArrayList<>();
    }
}
