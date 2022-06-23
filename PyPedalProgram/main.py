from tkinter import *
from Song import Song, SongWidget
from ScrollWidget import ScrollWidget

# TKinter variables
root = Tk()
root.title("Pedal Program")
root.geometry("850x500")
songListWidget = ScrollWidget(root, width=850)


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
        song_widgets.append(SongWidget(songListWidget.frame, songs[index], remove_command=remove_song))
    songListWidget.populate(song_widgets)
    songListWidget.pack(fill="both", expand=True)


# Program entry point
songs = []
addSongButton = Button(root, text="Add Song", command=add_song)


build()
addSongButton.pack(pady=20)
root.mainloop()
