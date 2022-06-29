import os.path
import shutil
import tkinter.messagebox
from tkinter import *
from tkinter import filedialog

from Pedal import *
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
        songs.append(Song(tracknum=len(songs) - 1, title=f"Track {len(songs) + 1}", filepath="[Not Chosen]"))
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
        song_widgets.append(SongWidget(songListWidget.frame, songs[index], remove_command=remove_song,
                                       file_select_command=ask_for_song_filename))
    songListWidget.populate(song_widgets)
    songListWidget.pack(fill="both", expand=True)


def make_gui():
    button_frame = Frame(root, height=50)
    add_song_button = Button(button_frame, text="Add Song", command=add_song, background="#C3BABA",
                             activebackground="#D4CBCB")
    finish_button = Button(button_frame, text="FINISH", command=finish, font="Bold", background="#78CDD7",
                           activebackground="#89dee8")

    instructions = ""
    instructions += "To add a song, press the \"Add Song\" button.\n"
    instructions += "Each song is composed of an MP3 file, a sequence of 4 notes, and an 8-character title.\n"
    instructions += "Use the drop-down selectors to select a note sequence for each song.\n"
    instructions += "NOTE: A note sequence cannot have the same note twice in a row. " \
                    "For example, A-B-A-B is acceptable, but A-A-B-A is not.\n"
    instructions += "Press the \"Finish\" button when you are done editing to upload your changes to the pedal.\n"

    instructions_title = Label(root, text="INSTRUCTIONS", font=("Default", 20))
    instructions_label = Label(root, text=instructions, justify="center")
    instructions_title.pack()
    instructions_label.pack()

    build()

    add_song_button.place(anchor=CENTER, relx=0.5, rely=0.5)
    finish_button.place(anchor=E, x=380, relx=0.5, rely=0.5)
    button_frame.pack(fill="x")

    root.mainloop()


def ask_for_song_filename(index):
    new_filename = filedialog.askopenfilename(title="Select a File", filetypes=[("MP3 Files", "*.mp3")])
    if new_filename:
        songs[index].filepath = new_filename
        rebuild()


def finish():
    # Check songs length
    if len(songs) <= 0:
        show_error(f"There must be at least one song in the list")
        return
    if len(songs) > MAX_NUM_SONGS:
        show_error(
            f"There are too many songs in the list ({len(songs)}). The maximum number of songs is {MAX_NUM_SONGS}.")
        return

    for song in songs:
        # Check for title validity
        if len(song.get_title()) > Song.TITLE_LENGTH:
            show_error(
                f"The title for Track {song.tracknum + 1} ({song.get_title()}) is too long. Please select a title that is 8 characters long or less.")
            return

        # Check for notes validity
        song_notes = song.get_notes()
        if "" in song_notes:
            show_error(
                f"Track {song.tracknum + 1} ({song.get_title()}) has at least one invalid note in its notes sequence. Please select a note for every slot in every song's notes sequence.")
            return
        for noteIndex in range(1, len(song_notes)):
            if song_notes[noteIndex] == song_notes[noteIndex - 1]:
                show_error(
                    f"Track {song.tracknum + 1} ({song.get_title()}) has the same note twice in a row in its notes sequence. A note cannot be repeated consecutively in a notes sequence.")
                return

        # Check for file location validity
        if not os.path.exists(song.filepath):
            show_error(
                f"File {song.filepath} does not exist. Please select a valid file for Track {song.tracknum + 1} ({song.get_title()}).")
            return

    # Send songs
    pedal.send_songs(songs)

    # Make temp folder to copy files to
    temp_dir = os.path.join(os.path.dirname(mp3_dir), "temp")
    os.mkdir(temp_dir)

    # Copy files to SD card
    for _songIndex, song in enumerate(songs):
        # Get new filename
        song_filename = os.path.basename(song.filepath)
        song_filename = str(_songIndex+1).zfill(4) + "_" + song_filename

        # Copy the file
        new_filepath = os.path.join(temp_dir, song_filename)
        shutil.copyfile(song.filepath, new_filepath)

    # Delete the old MP3 folder
    shutil.rmtree(mp3_dir)

    # Rename the temp folder to MP3
    os.rename(temp_dir, mp3_dir)


# Program entry point
songs = []
pedal = find_pedal()
if pedal is not None:
    # Get the current songs from pedal
    songs = pedal.get_songs()

    # Ask user to find the SD card
    tkinter.messagebox.showinfo("Select SD Card", "Please select the location of the MP3 folder on the SD card.")
    mp3_dir = filedialog.askdirectory(initialdir=os.getcwd(), title="Select a Folder")
    if not mp3_dir:
        pedal.close()
        exit(1)

    # Get current song filepaths
    for songIndex, _ in enumerate(songs):
        # Find file
        for _, _, files in os.walk(mp3_dir):
            for file in files:
                if file.startswith(str(songIndex+1).zfill(4)):
                    songs[songIndex].filepath = os.path.join(mp3_dir, file)
                    break

    # Use GUI to get new songs from user
    make_gui()

    # NOTE: Sending new songs to pedal starts in finish()

else:
    # TODO Pedal not found
    pass
