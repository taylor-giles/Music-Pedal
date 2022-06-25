from tkinter import *

POSSIBLE_NOTES = ["A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#"]


class Song:
    TITLE_LENGTH = 8

    def __init__(self, tracknum, title, filepath, notes=None):
        self.tracknum = tracknum
        self.filepath = filepath
        self._title = StringVar()
        self._title.set(title)
        self._notes = [StringVar(), StringVar(), StringVar(), StringVar()]

        if notes is not None:
            for index in range(len(self._notes)):
                self._notes[index].set(notes[index])

    def get_notes(self):
        return [note.get() for note in self._notes]

    def get_title(self):
        return self._title.get()


class SongWidget(Frame):
    def __init__(self, parent, song, remove_command, file_select_command):
        Frame.__init__(self, parent)
        self.song = song

        self.buttonsFrame = Frame(self)

        # Label for the track number
        self.numLabel = Label(self, text=f"{self.song.tracknum+1}")

        # Title label
        self.titleLabel = Label(self.buttonsFrame, text="Title: ")

        # Track title entry
        self.titleEntry = Entry(self.buttonsFrame, textvariable=song._title)
        # self.titleEntry.insert(0, self.song.title)

        # Filepath display
        self.filepathDisplay = Label(self, text=f"MP3 File:   {self.song.filepath}", justify="left")

        # Choose File Button
        self.chooseFileButton = Button(self.buttonsFrame, text="Choose MP3 File", command=lambda: file_select_command(self.song.tracknum), background="#C3BABA", activebackground="#D4CBCB")

        # Remove Song button
        self.removeButton = Button(self.buttonsFrame, text="Remove", command=lambda: remove_command(self.song.tracknum), background="#cc6163", activebackground="#dd7274")

        # Set up dropdowns for notes
        self.notesLabel = Label(self.buttonsFrame, text="Notes Sequence: ")
        self.noteDropdowns = []
        for note in self.song._notes:
            self.noteDropdowns.append(OptionMenu(self.buttonsFrame, note, *POSSIBLE_NOTES))
            self.noteDropdowns[-1].config(width=2, activebackground="#FAF4F7")

        # Pack title into grid
        self.titleLabel.grid(column=1, row=0, sticky="w")
        self.titleEntry.grid(column=2, row=0, columnspan=5, padx=(0, 30), ipady=4)

        # Pack dropdowns into grid
        self.notesLabel.grid(column=8, row=0, sticky="w")
        for index, dropdown in enumerate(self.noteDropdowns):
            dropdown.grid(column=11+index, row=0)

        # Pack buttons into grid
        self.chooseFileButton.grid(column=16, row=0, padx=(30, 0))
        self.removeButton.grid(column=17, row=0, padx=(10, 0))

        # Track number label
        self.numLabel.pack(side=LEFT, padx=(0, 20))

        # Buttons frame
        self.buttonsFrame.pack(side=TOP)

        # Filepath display
        self.filepathDisplay.pack(anchor="sw")
