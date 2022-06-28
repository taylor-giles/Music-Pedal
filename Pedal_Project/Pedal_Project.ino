#include <JC_Button.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <Wire.h>
#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Constants
#define SIZE_ADDRESS 0 //EEPROM address for number of songs
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RST -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_ADDRESS 0x3C //OLED display I2C address
#define MAX_SONGS 30 //The maximum number of songs that can be stored in EEPROM
#define TIMEOUT 2000 //Millis to wait for data to be available
#define CYCLE_LENGTH 8  //The number of times a frequency needs to be read to identify it as a note
#define HOLD_TIME 750 //The time, in millis, for a held button to register
#define DOUBLE_PRESS_TIME 300 //The maximum time, in millis, between double-press register

//Pins
#define BUTTON_PIN 8 //The pin that the footswitch uses
#define DF_TX_PIN 10 //The pin that connects to DFPlayer TX
#define DF_RX_PIN 11 //The pin that connects to DFPlayer RX
#define RED_PIN 3 //The pin to the red component of the RGB LED
#define GREEN_PIN 5 //The pin to the green component of the RGB LED
#define BLUE_PIN 6 //The pin to the blue component of the RGB LED
#define BUSY_PIN 4 //If this pin is LOW, the DFPlayer is playing a song, and vice-versa
#define REPEAT_PIN 7 //If this pin is LOW, when a song ends, it will be repeated (looped)
#define CONTINUE_PIN 9 //If this pin is LOW, when a song ends, the next song will begin

typedef struct Song {
  byte num;
  char notes[4];
  char title[9];
} Song;

/* VARIABLE DECLARATION*/
//Songs
Song tracks[MAX_SONGS];
Song currentTrack;
byte numSongs = 0;
const Song nullSong = {
  .num = -1,
  .notes = {'Z', 'Z', 'Z', 'Z'}
}; //Used in place of a null pointer
char notesIn[4] = "ZZZZ"; //The array that stores the input notes

//I2C Display
SSD1306AsciiWire display;

//DFPlayer
SoftwareSerial mySerial(DF_TX_PIN, DF_RX_PIN);
DFPlayerMini_Fast player;
boolean wasPlaying = false;

//Serial
const char PING_KEY[] = "Parrot";
const char CONFIRM_KEY[] = "Confirm";

//Input
const float MAX_FREQ = 400;
const float MIN_FREQ = 40;
const float BASEMAX = 80;
const float CERTAINTY = 0.025; //If a measured freq is within 2.5% of a note, identify as that note.
const float TOLERANCE = 2; //Hz needed to determine a consistent note
const Button ftsw(BUTTON_PIN);
boolean input = false;

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
const String NAMES[] = {"A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#"};
const char IDS[] =     {'A', 'H',  'B', 'C', 'I',  'D', 'J',  'E', 'F', 'K',  'G', 'L'};
float prevFreqs[CYCLE_LENGTH]; //Storage for previous frequencies (for note identification certainty)
int freqIndex = 0; //The current index for frequency processing
String note = "?"; //The name of the current saved note
char id = '?'; //The ID of the current saved note
int noteIndex = 0; //The index of the current note in the input array


/*
   Frequency Detection Variables, from Amanda Ghassaei's code
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
byte slopeTol = 3;//slope tolerance- adjust this if you need (was 3)
int timerTol = 10;//timer tolerance- adjust this if you need (was 10)

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30;//raise if you have a very noisy signal (was 30)

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
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUSY_PIN, INPUT);
  pinMode(REPEAT_PIN, INPUT_PULLUP);
  pinMode(CONTINUE_PIN, INPUT_PULLUP);
  
  //Set up footswitch
  ftsw.begin();

  //Set up DFPlayer
  mySerial.begin(9600);
  player.begin(mySerial);
  player.pause();
  player.volume(15);
  
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
  display.print("     ");
  display.setFont(X11fixed7x14B);
  display.print("   ");
  display.set2X();
  display.println("GILES");
  display.setFont(X11fixed7x14);
  display.set1X();
  display.println("   TECHNOLOGIES");

  //Get the number of songs stored in EEPROM
  EEPROM.get(SIZE_ADDRESS, numSongs);

  //Get the songs from EEPROM
  EEPROM.get(SIZE_ADDRESS + sizeof(numSongs), tracks);

  currentTrack = tracks[0];

  //Check for Edit Mode
  Serial.println(PING_KEY);
  delay(1000);
  if (getKeyFromSerial(PING_KEY)) {
    //Stop splash
    resetDisplay();
    editMode();
  }

  Serial.end();
 
  //Stop splash
  resetDisplay();
}

/**
   Loop function
*/
void loop() {
  ftsw.read(); //Read the state of the footswitch

  //If the button was held...
  if (ftsw.pressedFor(HOLD_TIME)) {
    //Reset the input variables
    noteIndex = 0;
    freqIndex = 0;
    id = '?';
    note = "?";
    
    //Enter input mode
    input = true;

    setColor(50,50,50);
    //display.println("Ready");

    //Wait until the button is released
    while (ftsw.isPressed()) {
      ftsw.read();
    }

    //Update display
    display.clear();
    display.invertDisplay(true);
    display.setFont(fixed_bold10x15);
    display.set1X();
    display.setCursor(4, 4);
    display.print("__ __ __ __");
    display.setCursor(4, 4);
  }
  
  //If the button was pressed, but not held...
  else if (ftsw.wasReleased()) {
    setColor(0,0,0);
    //If in input mode, cancel input
    if (input) {
      resetDisplay();
      input = false;

      //Restore playing display if track is still playing
      if(isPlaying()){
        playingDisplay(currentTrack);
      }
      
    } else {
      //Play/pause the player
      resetDisplay();
      delay(5);
      if (isPlaying()) {
        player.pause();
        delay(5);
        setColor(0,0,0);
        wasPlaying = false;
      } else {
        player.resume();
        playingDisplay(currentTrack);
        wasPlaying = true;
      }
    }
  }

  //If in input mode...
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

        //Update display
        display.setFont(fixed_bold10x15);
        display.set1X();
        display.print(note);

        //If the note is natural...
        if (id < 'H') {
          display.print(" "); //Add an extra space to compensate for smaller string length
        }
        display.print(" ");

        //If the correct number of notes was received...
        if (noteIndex == 4) { //4 is the index of the null character, which is added in this block
          input = false;

          //Reset display
          resetDisplay();

          //Find and play the corresponding song
          Song desired = findTrack(notesIn);

          //If the song was found...
          if (memcmp(desired.notes, nullSong.notes, sizeof(desired.notes)) != 0 && desired.num > 0) {
            currentTrack = desired;
            playTrack(currentTrack);
            delay(100);
          } 

          //If the song was not found...
          else {
            player.pause();
            delay(5);
            wasPlaying = false;

            //Update display
            setColor(255,0,0);
            display.println("Track not");
            display.println("found.\n");
            display.print("Notes:  ");
            for (int i = 0; i < noteIndex; i++) {
              for(int j = 0; j < 12; j++){
                if(IDS[j] == notesIn[i]){
                  display.print(NAMES[j]);
                  display.print("  ");
                  break;
                }
              }
            }
            delay(1500);
            display.clear();
            setColor(0,0,0);
          }

          //Reset variables
          noteIndex = 0;
          freqIndex = 0;
          id = '?';
        }
      }
      delay(10);
    }
  } else {
    //If a song was playing and it ended, react according to the repeat/continue switch
    if(!isPlaying() && wasPlaying){
      if(digitalRead(REPEAT_PIN) == LOW){
        //Loop this song
        playTrack(currentTrack);
      } else if(digitalRead(CONTINUE_PIN) == LOW){
        //Find and play the next song
        for(int i = 0; i < numSongs; i++){
          if(memcmp(currentTrack.notes, tracks[i].notes, sizeof(currentTrack.notes)) == 0){
            currentTrack = tracks[(i+1) % numSongs];
            playTrack(currentTrack); 
            break;
          }
        }
      } else {
        resetDisplay();
        setColor(0,0,0);
        wasPlaying = false;
      }
    }
  }
}


/**
 * Returns a boolean value indicating whether or not the DFPlayer is currently playing a track
 */
boolean isPlaying(){
  return (digitalRead(BUSY_PIN) == LOW);
}


/**
    Finds the song that corresponds to the given notes
*/
Song findTrack(char notes[4]) {
  for (int i = 0; i < numSongs; i++) {
    if (memcmp(notes, tracks[i].notes, sizeof(notes)) == 0) {
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
  delay(10);
  wasPlaying = true;

  //Update the display
  playingDisplay(song);
}


/**
   Updates the display when music is playing.
   Precondition: this should be called only when it is assumed that
   music is already playing. If music is not playing, this will print
   an error message.
*/
void playingDisplay(Song song) {
  resetDisplay();
  if(!isPlaying()){
    setColor(100,0,0);
    display.println("Error playing song.");
    display.println("\nMake sure your SD");
    display.println("card is inserted.");
    delay(1500);
  } else {
    setColor(0,0,100);
    display.println("Now Playing:");
    display.println(song.title);
    display.println();
    display.print("Notes:  ");
  
    //TODO: These sizeof() statements only work because each element of both arrays is one byte
    for (uint8_t i = 0; i < sizeof(song.notes); i++) {
      for (uint8_t j = 0; j < sizeof(IDS); j++) {
        if (IDS[j] == song.notes[i]) {
          display.print(NAMES[j]);
          display.print("  ");
        }
      }
    }
  }
  display.println();
}


/**
   Resets the display to the default settings
*/
void resetDisplay() {
  display.clear();
  display.invertDisplay(false);
  display.setFont(Verdana12);
  display.set1X();
}


/**
 * Sets the color of the RGB LED
 */
void setColor(uint8_t red, uint8_t green, uint8_t blue){
  analogWrite(RED_PIN, map(red, 0, 255, 0, 1023));
  analogWrite(GREEN_PIN, map(green, 0, 255, 0, 1023));
  analogWrite(BLUE_PIN, map(blue, 0, 255, 0, 1023));
}


/**
   Allows the pedal to communicate with the Java program
*/
void editMode() {
  display.clear();
  display.println("Edit mode");

  //Send number of songs
  for(byte i = 0; i < numSongs; i++){
    Serial.print("#");
    display.print("#");
  }
  Serial.println();

  //Send each song
  for(byte songIndex = 0; songIndex < numSongs; songIndex++){
    //Wait for confirmation
    delay(100);
    if(getKeyFromSerial(CONFIRM_KEY)){
      resetDisplay();
      //Send the title
      Serial.println(tracks[songIndex].title);
      display.println(tracks[songIndex].title);

      //Send the notes
      for(int noteIndex = 0; noteIndex < 4; noteIndex++){
        Serial.print(tracks[songIndex].notes[noteIndex]);
        display.print(tracks[songIndex].notes[noteIndex]);
      }
      Serial.println();
      display.println();
    } else {
      display.println("CONFIRMATION");
      display.println("NOT RECEIVED");
      delay(1000);
      return;
    }
  }

  //Wait for data to be available
  while(!Serial.available()){
    delay(1);
  }

  //Get new number of songs
  byte newNumSongs = 0;
  while(Serial.available()){
    if(Serial.read() == '#'){
      newNumSongs++;
    }
  }

  //TODO: Read in songs
  for(byte songIndex = 0; songIndex < newNumSongs; songIndex++){
    //Send confirmation
    Serial.println(CONFIRM_KEY);
    delay(250);

    //TODO: Get title
  }
    
  
  
  display.clear();
  display.println("Success!\n");
  display.println("Don't forget to");
  display.println("put your SD card back!");
  delay(500000);
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
  delay(50);

  //While reading data, check against key
  int index = 0;
  while(Serial.available()){
    if(key[index++] != Serial.read()){
      return false;
    }
  }

  //Make sure key is not longer than input
  return key[index] == '\0';
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
    PORTB &= B11011111;//turn off clipping indicator LED
    clipping = 0;
  }
}
