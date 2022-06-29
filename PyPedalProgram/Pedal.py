import time
import serial
from serial.tools import list_ports
from Song import Song, POSSIBLE_NOTES

PING_KEY = "Parrot"
CONFIRM_KEY = "Confirm"
NOTES_IDS = ['A', 'H', 'B', 'C', 'I', 'D', 'J', 'E', 'F', 'K', 'G', 'L']


# Wrapper class on serial.Serial for communicating with the pedal
class Pedal:
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1)

    def getline(self):
        time.sleep(0.25)
        return self.ser.readline().replace(b'\r', b'').replace(b'\n', b'').decode('utf-8')

    def send(self, msg):
        self.ser.write(msg.encode('utf-8'))

    def close(self):
        self.ser.close()

    def got_key(self, key):
        return self.getline() == key

    # Communicates with the pedal over Serial to retrieve title and notes data for each song
    def get_songs(self):
        # Get number of songs (one # character is received for each song)
        num_songs = len(self.getline())
        print(f"Num songs: {num_songs}")
        songs = []
        for i in range(num_songs):
            # Send confirmation
            self.send(CONFIRM_KEY)
            print()

            # Get this song's title
            title = self.getline()
            print(f"Title: {title}")

            # Get this song's notes
            notes_str = self.getline()
            print(f"Notes: {notes_str}")
            notes = [''] * len(notes_str)
            for note_index in range(len(notes_str)):
                notes[note_index] = POSSIBLE_NOTES[NOTES_IDS.index(notes_str[note_index])]

            songs.append(Song(title=title, notes=notes, tracknum=len(songs) - 1, filepath="[Not Chosen]"))
        return songs

    # Sends title and notes data for each song to the pedal
    def send_songs(self, songs):
        # Send the number of songs (send one # character for each song)
        self.send("#" * len(songs))

        # Send each song
        for song in songs:
            # Wait for confirmation
            if self.got_key(CONFIRM_KEY):
                # Send this song's title
                self.send(song.get_title().ljust(Song.TITLE_LENGTH, '\0'))

                # Send this song's notes
                notes_str = ""
                for note in song.get_notes():
                    notes_str += NOTES_IDS[POSSIBLE_NOTES.index(note)]
                self.send(notes_str)


# Searches through all available serial ports to find the Pedal.
# If the pedal is found (a port which sends the PING_KEY), it is returned.
# Otherwise, None is returned.
def find_pedal():
    # Loop through all ports to find the right one
    ports = serial.tools.list_ports.comports()
    for port, _, _ in ports:
        # Open serial port
        attempt = Pedal(port)

        # Wait for Arduino bootloader
        time.sleep(1)

        # Listen for ping
        if attempt.got_key(PING_KEY):
            print("Received ping")

            # Respond to ping with ping
            attempt.send(PING_KEY)
            print("Sent ping")

            # Return the open serial object for this port
            return attempt
        else:
            attempt.close()
    return None


if __name__ == '__main__':
    pedal = find_pedal()
    if pedal is not None:
        # Get the current songs
        current_songs = pedal.get_songs()

        # TODO: Get the new songs from GUI

        # TODO: Send new songs to pedal

    else:
        # TODO Pedal not found
        pass
