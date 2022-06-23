from tkinter import *
from tkinter import filedialog
from Song import Song, SongWidget
from ScrollWidget import ScrollWidget

# TKinter variables
root = Tk()
root.title("Pedal Program")
root.geometry("850x500")
songListWidget = ScrollWidget(root)


def add_song():
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


# Program entry point
songs = []
addSongButton = Button(root, text="Add Song", command=add_song)

instructions = ""
instructions += "To add a song, press the \"Add Song\" button.\n"
instructions += "Each song is composed of an MP3 file, a sequence of 4 notes, and an 8-character title.\n"
instructions += "Use the drop-down selectors to select a note sequence for each song.\n"
instructions += "NOTE: A note sequence cannot have the same note twice in a row. For example, A-B-A-B is acceptable, but A-A-B-A is not.\n"
instructions += "Press the \"Finish\" button when you are done editing to upload your changes to the pedal.\n"

instructionsTitle = Label(root, text="INSTRUCTIONS", font=("Default", 20))
instructionsLabel = Label(root, text=instructions, justify="center")
instructionsTitle.pack()
instructionsLabel.pack()

build()
addSongButton.pack(pady=20)
root.mainloop()
