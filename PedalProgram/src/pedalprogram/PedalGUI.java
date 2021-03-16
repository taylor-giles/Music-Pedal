/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package pedalprogram;

import java.awt.BorderLayout;
import java.awt.Button;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;

/**
 *
 * @author super
 */
public class PedalGUI extends javax.swing.JFrame {
    private List<Song> songs = new ArrayList<>();
    private JPanel songContainer = new JPanel();
    private JPanel scrollPanel = new JPanel();
    private JScrollPane scrollPane = new JScrollPane();
    private JButton addSongButton = new JButton("Add Song");
    private JButton cancelButton = new JButton("Cancel");
    private JButton doneButton = new JButton("Finish");
    private boolean ready = false;
    private ArrayList<Song> finalList = new ArrayList<>();
    

    /**
     * Creates new form PedalGUI
     */
    public PedalGUI() {
        initComponents();
    }
    
    public PedalGUI(ArrayList<Song> songList){
        initComponents();
        songs = songList;
        setTitle("Pedal Songs Editor");
        setLayout(new FlowLayout());
        
        JPanel finishButtonLayout = new JPanel();
        finishButtonLayout.setPreferredSize(new Dimension(820, 25));
        finishButtonLayout.setLayout(new BorderLayout(10, 10));
        finishButtonLayout.add(cancelButton, BorderLayout.LINE_START);
        finishButtonLayout.add(doneButton, BorderLayout.LINE_END);
        add(finishButtonLayout);
        
        addSongButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                addSongButtonActionPerformed(evt);
            }
        });
        
        cancelButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                System.exit(1);
            }
        });
        
        doneButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                doneButtonActionPerformed(evt);
            }
        });
        
        init();
        
        setSize(1000,750);
        setVisible(true);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setResizable(false);
        revalidate();
        repaint();
    }
    
    private void init(){
        songContainer.removeAll();
        scrollPanel.removeAll();
        for(int i = 0; i < songs.size(); i++){
            songs.get(i).setIndex(i+1);
            songContainer.add(new SongPanel(this, songs.get(i)));
        }
        scrollPanel.add(songContainer);
        scrollPanel.add(addSongButton);
        songContainer.setPreferredSize(new Dimension(820, songs.size() * SongPanel.HEIGHT));
        scrollPanel.setPreferredSize(new Dimension(820, songContainer.getHeight()));
        scrollPanel.revalidate();
        
        getContentPane().remove(scrollPane);
        scrollPane = new JScrollPane(scrollPanel,
                                         JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                                         JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        scrollPane.setPreferredSize(new Dimension(850, 575));
        add(scrollPane);
        
        revalidate();
        repaint();
    }
    
    public void removeSong(int index){
        songs.remove(index - 1);
        for(Component comp : songContainer.getComponents()){
            if(comp instanceof SongPanel){
                SongPanel panel = (SongPanel) comp;
                if(panel.getSong().getIndex() == index){
                    songContainer.remove(panel);
                }
            }
        }
        songContainer.revalidate();
        songContainer.setPreferredSize(new Dimension(820, songs.size() * SongPanel.HEIGHT));
        scrollPanel.setPreferredSize(new Dimension(820, songContainer.getHeight()));
        scrollPane.revalidate();
        revalidate();
        repaint();
    }
    
    private void addSongButtonActionPerformed(java.awt.event.ActionEvent evt) { 
        Song newSong = new Song();
        newSong.setIndex(songs.size() + 1);
        songs.add(newSong);
        songContainer.add(new SongPanel(this, newSong));
        songContainer.setPreferredSize(new Dimension(820, songs.size() * SongPanel.HEIGHT));
        scrollPanel.setPreferredSize(new Dimension(820, songContainer.getHeight()));
        scrollPane.revalidate();
        revalidate();
        repaint();
    } 
    
    private void doneButtonActionPerformed(java.awt.event.ActionEvent evt) {
        finalList = new ArrayList<>();
        for(Component comp : songContainer.getComponents()){
            if(comp instanceof SongPanel){
                SongPanel panel = (SongPanel) comp;
                Song song = panel.getSong();
                song.setIndex(panel.getIndex());
                if(song.getFile() == null || !song.getFile().exists()){
                    JOptionPane.showMessageDialog(null, "File: " + song.getFilepath() + " could not be located.", "Error", JOptionPane.ERROR_MESSAGE);
                }
                song.setFilepath(panel.getFilename());
                try {
                    //Report adjacent identical notes
                    for(int i = 1; i < panel.getNotes().length; i++){
                        if(panel.getNotes()[i] == panel.getNotes()[i-1]){
                            JOptionPane.showMessageDialog(null, "Adjancent notes cannot be identical.\nPlease change the notes in song " + panel.getIndex() + ".", "Error", JOptionPane.ERROR_MESSAGE);
                            return;
                        } 
                    }
                    
                    //Report songs with identical notes
                    for(Song s : finalList){
                        if(s.getNotes().equals(panel.getNotes())){
                            JOptionPane.showMessageDialog(null, "Each song must have a unique set of notes.\nPlease change the notes in song " + panel.getIndex() + ".", "Error", JOptionPane.ERROR_MESSAGE);
                            return;
                        }
                    }
                    
                    //Set notes
                    song.setNotes(panel.getNotes());
                    
                } catch(ArrayIndexOutOfBoundsException ex){
                    //Report blank notes
                    JOptionPane.showMessageDialog(null, "All notes for all songs must be selected", "Error", JOptionPane.ERROR_MESSAGE);
                    return;
                }
                finalList.add(song);
                System.out.println("Song added");
            }
        }
        System.out.println("Ready");
        ready = true;
    }
    
    public boolean isReady(){
        return ready;
    }
    
    public ArrayList<Song> getData(){
        return finalList;
    }

    /**
     * This method is called from within the constructor to initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is always
     * regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        jPanel1 = new javax.swing.JPanel();
        jLabel1 = new javax.swing.JLabel();

        setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

        jLabel1.setFont(new java.awt.Font("Tahoma", 1, 18)); // NOI18N
        jLabel1.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel1.setText("Edit Songs");

        javax.swing.GroupLayout jPanel1Layout = new javax.swing.GroupLayout(jPanel1);
        jPanel1.setLayout(jPanel1Layout);
        jPanel1Layout.setHorizontalGroup(
            jPanel1Layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(jPanel1Layout.createSequentialGroup()
                .addContainerGap()
                .addComponent(jLabel1, javax.swing.GroupLayout.DEFAULT_SIZE, 850, Short.MAX_VALUE)
                .addContainerGap())
        );
        jPanel1Layout.setVerticalGroup(
            jPanel1Layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(jPanel1Layout.createSequentialGroup()
                .addContainerGap()
                .addComponent(jLabel1)
                .addContainerGap(javax.swing.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
        );

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(layout.createSequentialGroup()
                .addContainerGap()
                .addComponent(jPanel1, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.PREFERRED_SIZE)
                .addContainerGap())
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(layout.createSequentialGroup()
                .addContainerGap()
                .addComponent(jPanel1, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.PREFERRED_SIZE)
                .addContainerGap())
        );

        pack();
    }// </editor-fold>//GEN-END:initComponents

    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) {
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

        /* Create and display the form */
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                ArrayList<Song> temp = new ArrayList<>();
                for(int i = 0; i < 15; i++){
                    Song newSong = new Song();
                    newSong.setIndex(i+1);
                    temp.add(newSong);
                }
                new PedalGUI(temp).setVisible(true);
            }
        });
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JLabel jLabel1;
    private javax.swing.JPanel jPanel1;
    // End of variables declaration//GEN-END:variables
}
