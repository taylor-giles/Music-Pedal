#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <Wire.h>
#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define SIZE_ADDRESS 0 //EEPROM address for number of songs
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RST -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_ADDRESS 0x3C //OLED display I2C address
#define MAX_SONGS 70 //The maximum number of songs that can be stored in EEPROM
#define TIMEOUT 2000 //Millis to wait for data to be available
#define CYCLE_LENGTH 8  //The number of times a frequency needs to be read to identify it as a note
#define BUTTON_PIN 8 //The pin that the footswitch uses
#define HOLD_TIME 1250 //The time, in millis, for a hold to register
#define DOUBLE_PRESS_TIME 300 //The maximum time, in millis, between double-press register

typedef struct Song {
  char notes[5];
  int num;
} Song;

//Declarations
//Songs
Song tracks[MAX_SONGS];
int numSongs = 0;
const Song nullSong = { .notes = {'Z', 'Z', 'Z', 'Z', '\0'}, .num = -1 }; //Used in place of a null pointer
char notesIn[5] = "ZZZZ"; //The array that stores the input notes

//I2C Display
SSD1306AsciiWire display;

//DFPlayer
SoftwareSerial mySerial(10, 11);
DFPlayerMini_Fast player;

//Serial
const char PING_KEY[] = "Parrot";
const char CONFIRM_KEY[] = "Confirm";

//Input
const float MAX_FREQ = 400;
const float MIN_FREQ = 40;
const float BASEMAX = 80;
const float CERTAINTY = 0.025; //If a measured freq is within 2.5% of a note, identify as that note.
const float TOLERANCE = 2; //Hz needed to determine a consistent note
boolean input = false;
long pressedAt = 0;  //Time in millis that the footswitch was pressed (used for press length detection)

//Note identification
const float E = 41.2034;
const float F = 43.6535;
const float Gb = 46.2493;
const float G = 48.9994;
const float Ab = 51.9131;
const float A = 55;
const float Bb = 58.2705;
const float B = 61.7354;
const float C = 65.4064;
const float Db = 69.2957;
const float D = 73.4162;
const float Eb = 77.7817;
const float NOTES[] = {A, Bb, B, C, Db, D, Eb, E, F, Gb, G, Ab};
const String NAMES[] = {"A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"};
const char IDS[] = {'A', 'H', 'B', 'C', 'I', 'D', 'J', 'E', 'F', 'K', 'G', 'L'};
float prevFreqs[CYCLE_LENGTH]; //Storage for previous frequencies
int freqIndex = 0; //The current index for frequency processing
String note = "?"; //The name of the current saved note
char id = '?'; //The ID of the current saved note
int noteIndex = 0; //The index of the current note in the input array


/*
   Frequency Detection Variables
*/
//clipping indicator variables
boolean clipping = 0;

//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//Keeps time and sends vales to store in timer[] occasionally
int timer[10]; //Storage for timing of events
int slope[10]; //Storage for slope of events
unsigned int totalTimer; //Used to calculate period
unsigned int period; //Storage for period of wave
byte index = 0; //Current storage index
float frequency; //Storage for frequency calculations
int maxSlope = 0; //Used to calculate max slope as trigger point
int newSlope; //Storage for incoming slope data

//variables for decided whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30;//raise if you have a very noisy signal

/* END VARIABLE DECLARATION */


/**
   Setup function
*/
void setup() {
  Serial.begin(115200);

  //Set up audio frequency input process
  freqInit();

  //Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  pinMode(3, OUTPUT);
  analogWrite(3, 127);

  //Set up DFPlayer
  mySerial.begin(9600);
  player.begin(mySerial);
  player.pause();
  player.volume(20);
  //Serial.println("Player ready.");

  //Set up display
  Wire.begin();
  Wire.setClock(400000L);
#if RST_PIN >= 0
  display.begin(&Adafruit128x64, OLED_ADDRESS, OLED_RST);
#else
  display.begin(&Adafruit128x64, OLED_ADDRESS);
#endif
  display.clear();

  //Splash
  display.invertDisplay(true);
  display.setFont(Iain5x7);
  display.set1X();
  display.println();
  display.print("    ");
  display.setFont(X11fixed7x14B);
  display.print(" ");
  display.set2X();
  display.println("A G T G");
  display.setFont(X11fixed7x14);
  display.set1X();
  display.println(" P  E  D  A  L  S");


  //Get the number of songs stored in EEPROM
  EEPROM.get(SIZE_ADDRESS, numSongs);
  //Serial.print("Number of Songs: ");
  //Serial.println(numSongs, DEC);

  //Get the songs from EEPROM
  EEPROM.get(SIZE_ADDRESS + sizeof(numSongs), tracks);
  //Serial.println("Songs retrieved.");

  //Check for Edit Mode
  Serial.println(PING_KEY);
  delay(1000);
  if (getKeyFromSerial(CONFIRM_KEY)) {
    //Stop splash
    display.clear();
    display.invertDisplay(false);
    display.setFont(Verdana12);
    display.set1X();

    editMode();
  }

  //Stop splash
  display.clear();
  display.invertDisplay(false);
  display.setFont(Verdana12);
  display.set1X();
}

/**
   Loop function
*/
void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Pressed");
    display.clear();
    display.setFont(fixed_bold10x15);
    display.set1X();
    delay(100);
    //Check for double-press
    if (millis() - pressedAt < DOUBLE_PRESS_TIME) {
      display.println("Double press");
    } else {
      pressedAt = millis();
      input = false;

      //If the button is held, proceed to input stage
      while (digitalRead(BUTTON_PIN) == LOW) {
        if ((millis() - pressedAt) >= HOLD_TIME) {
          input = true;
          Serial.println("Input");
          //Update display
          display.setCursor(0, 0);
          display.println("Input\n");
        }
      }
      delay(50);

      //If the button was not held, play/pause the song
      if (!input) {
        if (player.isPlaying()) {
          player.pause();
        } else {
          player.resume();
        }
      }
    }
  }

  if (input) {
    //Check for input
    checkClipping();

    //If a frequency is found...
    if (checkMaxAmp > ampThreshold) {
      frequency = 38462 / float(period); //calculate frequency (timer rate/period)

      //If the found frequency is in range...
      if (frequency > MIN_FREQ && frequency < MAX_FREQ) {
        //If no frequency was found previously...
        if (freqIndex == 0) {
          prevFreqs[0] = frequency; //Make this frequency the next one to analyze
          freqIndex++;
        } else { //If there is already a frequency being analyzed...
          //If this frequency matches the previous one...
          if (abs(frequency - prevFreqs[freqIndex - 1]) < TOLERANCE) {
            //If the cycle is complete...
            if (freqIndex >= CYCLE_LENGTH) {
              freqIndex = 0; //Reset index

              //Calculate the average of the recorded measurements
              float sum = 0.0;
              float avg = 0.0;
              for (int i = 0; i < CYCLE_LENGTH; i++) {
                sum = sum + prevFreqs[i];
              }
              avg = sum / CYCLE_LENGTH;

              //Find the frequency of this note at Octave 1
              float measured = avg;
              while (measured > BASEMAX) {
                measured = measured / 2.0;
              }

              //Match this frequency to a note
              note = "?";
              id = '?';
              for (int i = 0; i < 12; i++) {
                //If this frequency is within the interval of certainty of a note...
                if (NOTES[i] * (1.0 + CERTAINTY) > measured && NOTES[i] * (1.0 - CERTAINTY) < measured) {
                  //Save that note as the measured note
                  note = NAMES[i];
                  id = IDS[i];
                  break;
                }
              }
            } else { //If the cycle is not complete...
              prevFreqs[freqIndex] = frequency; //Save the measured frequency to the array
              freqIndex++;
            }
          } else { //If this frequency does not match the previous one...
            freqIndex = 0; //Reset the counter
          }
        }
      }

      //If the current note is valid and is not the same as the previous one, or it is the first one...
      if (id != '?' && (noteIndex == 0 || id != notesIn[noteIndex - 1])) {
        //Process the current note
        Serial.println(note);
        notesIn[noteIndex] = id;
        noteIndex++;

        //Display the new note
        display.set1X();
        display.print(note);
        display.print(" ");

        //If the correct number of notes was received...
        if (noteIndex == 4) {
          input = false;
          notesIn[noteIndex] = '\0'; //Indicate that the input is finished
          noteIndex = 0;
          freqIndex = 0;
          id = '?';

          //Find and play the corresponding song
          Song desired = findTrack(notesIn);
          Serial.println(desired.num);
          display.setFont(Verdana12);
          display.set1X();
          if (strcmp(desired.notes, nullSong.notes) != 0 && desired.num > 0) {
            playTrack(desired);
            delay(1000);
          } else {
            player.pause();
            display.clear();
            display.set2X();
            display.println("Track not");
            display.println("found.");
            delay(1000);
            display.clear();
          }
        }
      }
      delay(10);
    }
  }
}

/**
    Finds the song that corresponds to the given notes
*/
Song findTrack(char notes[5]) {
  for (int i = 0; i < numSongs; i++) {
    if (strcmp(notes, tracks[i].notes) == 0) {
      return tracks[i];
    }
  }
  return nullSong;
}

/**
   Plays the track that corresponds to the given index,
   and updates display accordingly.
*/
void playTrack(Song song) {
  //Play the song
  player.play(song.num);

  //Update the display
  display.clear();
  display.set1X();
  display.println("Now Playing:");
  display.print("Track ");
  display.println(song.num);
  display.println();
  display.print("Notes: ");
  for (int i = 0; i < sizeof(song.notes); i++) {
    display.print(song.notes[i]);
    display.print(" ");
  }
  display.println();
}

/**
   Allows the pedal to communicate with the Java program
*/
void editMode() {
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  display.clear();
  display.println("Edit mode");
  Serial.println(CONFIRM_KEY);
  delay(50);

  //Wait for program to send data
  while(!Serial.available()){}
  delay(500);

  //Get new number of songs
  int newNumSongs = 0;
  char in;
  while(Serial.available()){
    in = Serial.read();
    display.print(in);
    if(in == '#'){
      newNumSongs++;
    } 
  }
  display.println(newNumSongs);
  Serial.println(CONFIRM_KEY);

  delay(500);

  display.clear();

  //Read in the new songs
  Song newSongs[newNumSongs];
  for (int i = 0; i < newNumSongs; i++) {
    newSongs[i] = getSong(i + 1);
    display.print(newSongs[i].num);
    display.print(" ");
    display.println(newSongs[i].notes);
  }

  delay(50);

  //Write new songs to EEPROM
  //TODO Uncomment these lines
//  EEPROM.put(0, newNumSongs);
//  EEPROM.put(sizeof(numSongs), newSongs);
}


/**
   Gets song data from the serial port
   Gets ONLY the notes - the index is determined by the order
    in which the songs are sent
*/
Song getSong(int index) {
  Song newSong;
  newSong.num = index;
  for (int i = 0; i < 4; i++) {
    newSong.notes[i] = Serial.read();
    delay(50);
  }
  newSong.notes[4] = '\0';
  return newSong;
}

/**
   Checks to see if the requested String was sent over the serial port
*/
boolean getKeyFromSerial(char key[]) {

  //Wait for data to become available
  int start = millis();
  while (!Serial.available()) {
    if (millis() - start > TIMEOUT) {
      return false;
    }
  }

  //Access data if it is available
  char input[sizeof(key)] = "";
  int c = 0;
  while (Serial.available()) {
    input[c] = Serial.read();
    c++;
    delay(50);
  }

  return strcmp(key, input) == 0;
}

/***EVERYTHING BELOW THIS POINT WAS TAKEN IN WHOLE OR IN PART FROM AMANDA GHASSAEI'S TUTORIAL ON ARDUINO FREQUENCY INPUT***/

/**
   Sets up the frequency input process, using Amanda Ghassaei's code
*/
void freqInit() {
  cli();//disable interrupts

  //set up continuous sampling of analog pin 0 at 38.5kHz

  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;

  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only

  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements

  sei();//enable interrupts
}

/**
   Handles processing when a new value is ready on the ADC, using Amanda Ghassaei's code
*/
ISR(ADC_vect) {//when new ADC value ready

  PORTB &= B11101111;//set pin 12 low
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  if (prevData < 127 && newData >= 127) { //if increasing and crossing midpoint
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope - maxSlope) < slopeTol) { //if slopes are ==
      //record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0) { //new max slope just reset
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0] - timer[index]) < timerTol && abs(slope[0] - newSlope) < slopeTol) { //if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i = 0; i < index; i++) {
          totalTimer += timer[i];
        }
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else { //crossing midpoint but not match
        index++;//increment index
        if (index > 9) {
          reset();
        }
      }
    }
    else if (newSlope > maxSlope) { //if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else { //slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch > 9) {
        reset();
      }
    }
  }

  if (newData == 0 || newData == 1023) { //if clipping
    PORTB |= B00100000;//set pin 13 high- turn on clipping indicator led
    clipping = 1;//currently clipping
  }

  time++;//increment timer at rate of 38.5kHz

  ampTimer++;//increment amplitude timer
  if (abs(127 - ADCH) > maxAmp) {
    maxAmp = abs(127 - ADCH);
  }
  if (ampTimer == 1000) {
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }

}

/**
   Resets frequency input variables, using Amanda Ghassaei's code
*/
void reset() { //clear out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}

/**
   Checks if the frequency input is being clipped and acts accordingly, using Amanda Ghassaei's code
*/
void checkClipping() { //manage clipping indicator LED
  if (clipping) { //if currently clipping
    PORTB &= B11011111;//turn off clipping indicator led
    clipping = 0;
  }
}
