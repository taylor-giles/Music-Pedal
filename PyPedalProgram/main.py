import os.path
import tkinter.messagebox
from tkinter import *
from tkinter import filedialog
from Song import Song, SongWidget
from ScrollWidget import ScrollWidget

MAX_NUM_SONGS = 35

# TKinter variables
root = Tk()
root.title("Pedal Program")
root.geometry("850x500")
songListWidget = ScrollWidget(root, width=850)


def show_error(msg):
    tkinter.messagebox.showerror("Error", msg)


def add_song():
    if len(songs) < MAX_NUM_SONGS:
        songs.append(Song(tracknum=len(songs)-1, title=f"Track {len(songs)+1}", filepath="[Not Chosen]"))
        rebuild()


def remove_song(index=-1):
    songs.remove(songs[index])
    rebuild()


def rebuild():
    songListWidget.empty()
    build()


def build():
    song_widgets = []
    for index, _ in enumerate(songs):
        songs[index].tracknum = index
        song_widgets.append(SongWidget(songListWidget.frame, songs[index], remove_command=remove_song, file_select_command=ask_for_song_filename))
    songListWidget.populate(song_widgets)
    songListWidget.pack(fill="both", expand=True)


def ask_for_song_filename(index):
    new_filename = filedialog.askopenfilename(title="Select a File", filetypes=[("MP3 Files", "*.mp3")])
    if new_filename:
        songs[index].filepath = new_filename
        rebuild()


def finish():
    # Check songs length
    if len(songs) > MAX_NUM_SONGS:
        show_error(f"There are too many songs in the list ({len(songs)}). The maximum number of songs is {MAX_NUM_SONGS}.")
        return

    for song in songs:
        # Check for title validity
        if len(song.get_title()) > Song.TITLE_LENGTH:
            show_error(f"The title for Track {song.tracknum + 1} ({song.get_title()}) is too long. Please select a title that is 8 characters long or less.")
            return

        # Check for notes validity
        song_notes = song.get_notes()
        if "" in song_notes:
            show_error(f"Track {song.tracknum + 1} ({song.get_title()}) has at least one invalid note in its notes sequence. Please select a note for every slot in every song's notes sequence.")
            return
        for noteIndex in range(1, len(song_notes)):
            if song_notes[noteIndex] == song_notes[noteIndex-1]:
                show_error(f"Track {song.tracknum + 1} ({song.get_title()}) has the same note twice in a row in its notes sequence. A note cannot be repeated consecutively in a notes sequence.")
                return

        # Check for file location validity
        if not os.path.exists(song.filepath):
            show_error(f"File {song.filepath} does not exist. Please select a valid file for Track {song.tracknum + 1} ({song.get_title()}).")
            return


# Program entry point
songs = []
buttonFrame = Frame(root, height=50)
addSongButton = Button(buttonFrame, text="Add Song", command=add_song, background="#C3BABA", activebackground="#D4CBCB")
finishButton = Button(buttonFrame, text="FINISH", command=finish, background="#78CDD7", activebackground="#89dee8", font="Bold")

instructions = ""
instructions += "To add a song, press the \"Add Song\" button.\n"
instructions += "Each song is composed of an MP3 file, a sequence of 4 notes, and an 8-character title.\n"
instructions += "Use the drop-down selectors to select a note sequence for each song.\n"
instructions += "NOTE: A note sequence cannot have the same note twice in a row. " \
                "For example, A-B-A-B is acceptable, but A-A-B-A is not.\n"
instructions += "Press the \"Finish\" button when you are done editing to upload your changes to the pedal.\n"

instructionsTitle = Label(root, text="INSTRUCTIONS", font=("Default", 20))
instructionsLabel = Label(root, text=instructions, justify="center")
instructionsTitle.pack()
instructionsLabel.pack()

build()

addSongButton.place(anchor=CENTER, relx=0.5, rely=0.5)
finishButton.place(anchor=E, x=380, relx=0.5, rely=0.5)
buttonFrame.pack(fill="x")

root.mainloop()
