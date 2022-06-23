from tkinter import *

POSSIBLE_NOTES = ["A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#"]


class Song:
    def __init__(self, tracknum, title, filepath):
        self.tracknum = tracknum
        self.title = title
        self.filepath = filepath


class SongWidget(Frame):
    def __init__(self, parent, song, remove_command, file_select_command):
        Frame.__init__(self, parent)
        self.song = song

        # Label for the track number
        self.numLabel = Label(self, text=f"{self.song.tracknum+1}")

        # Title label
        self.titleLabel = Label(self, text="Title: ")

        # Track title entry
        self.titleEntry = Entry(self)
        self.titleEntry.insert(0, self.song.title)

        # Filepath display
        self.filepathDisplay = Label(self, text=f"MP3 File:   {self.song.filepath}")

        # Choose File Button
        self.chooseFileButton = Button(self, text="Choose MP3 File", command=lambda: file_select_command(self.song.tracknum))

        # Remove Song button
        self.removeButton = Button(self, text="Remove", command=lambda: remove_command(self.song.tracknum))

        # Set up dropdowns for notes
        self.notesLabel = Label(self, text="Notes Sequence: ")
        self.notes = [StringVar(self), StringVar(self), StringVar(self), StringVar(self)]
        self.noteDropdowns = []
        for note in self.notes:
            self.noteDropdowns.append(OptionMenu(self, note, *POSSIBLE_NOTES))
            self.noteDropdowns[-1].config(width=2)

        # Pack title into grid
        self.titleLabel.grid(column=1, row=0, sticky="w")
        self.titleEntry.grid(column=2, row=0, columnspan=5, padx=(0, 30), ipady=4)

        # Pack dropdowns into grid
        self.notesLabel.grid(column=8, row=0, sticky="w")
        for index, dropdown in enumerate(self.noteDropdowns):
            dropdown.grid(column=11+index, row=0)

        # Pack filepath into grid
        self.filepathDisplay.grid(column=1, row=1, columnspan=200, sticky="w")

        # Pack buttons into grid
        self.chooseFileButton.grid(column=16, row=0, padx=(30, 0))
        self.removeButton.grid(column=17, row=0, padx=(10, 0))

        # Track number label
        self.numLabel.grid(column=0, row=0, rowspan=2, padx=(0, 20))
