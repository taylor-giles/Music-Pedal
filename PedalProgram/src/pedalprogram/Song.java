/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package pedalprogram;

import java.io.File;
import java.io.Serializable;
import java.util.Arrays;

/**
 * The <code>Song</code> class allows for the storage of information that
 * corresponds to a song to be stored in the pedal.
 * 
 * @author super
 */
public class Song implements Serializable{
    public final static int DIGITS = 4;
    public final static int NOTES = 4;
    public final static String[] NAMES = {"A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"};
    public final static char[] IDS = {'A', 'H', 'B', 'C', 'I', 'D', 'J', 'E', 'F', 'K', 'G', 'L'};
    
    private char[] notes = new char[4];
    private int num = 0;
    private File file;
    private String filepath = "";
    
    public Song(String notes, int index){
        this.notes = notes.toCharArray();
        num = index;
    }
    
    public Song(char[] notes, int index){
        this.notes = notes;
        num = index;
    }

    Song() {}
    
    public int getIndex(){
        return num;
    }
    
    public void setIndex(int index){
        num = index;
    }
    
    public String getNotesStr(){
        StringBuilder output = new StringBuilder();
        for(char c : notes){
            output.append(c);
        }
        return output.toString();
    }
    
    public char[] getNotes(){
        return notes;
    }
    
    public void setNotes(char[] notes){
        this.notes = notes;
    }
    
    public String getFilepath(){
        return filepath;
    }
    
    public void setFile(File file){
        this.file = file;
        setFilepath(file.getAbsolutePath());
    }
    
    public File getFile(){
        return file;
    }
    
    public void setFilepath(String filepath){
        this.filepath = filepath;
    }
    
    public static int getIndexOfNote(char note){
        for(int i = 0; i < IDS.length; i++){
            if(IDS[i] == note){
                return i;
            }
        }
        return -1;
    }
}
