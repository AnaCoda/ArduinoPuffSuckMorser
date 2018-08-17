/*
Based on tutorial at http://mypetarduino.com/ReadCode/readCode.01.html

WB7FHC's Morse Code Decoder v. 1.1 (c) 2014, Budd Churchward - WB7FHC This is an Open Source Project http://opensource.org/licenses/MIT
Search YouTube for 'WB7FHC' to see several videos of this project as it was developed.

and https://vk8da.com/an-arduino-iambic-keyer/, which further gives credit to http://www.jel.gr/cw-mode/iambic-keyer-with-arduino-in-5-minutes/
(looks like a dead page).

No licenses found for the latter 2, please inform me if you find one!
*/

// Iambic keyer
#define DIT_PIN 8
#define DAH_PIN 10
#define LED 13
#define SPEAKER 11

// keyer state
enum
{
  IDLE,
  DIT,
  DAH,
};

int dit, dah;
int state;

/* The way we are translating morse code is we have a start bit (1) and for each dot, we shift
 * our number left. For each dash, we add one and shift our number left. The seemingly random
 * letterset array has the corresponding letters.
 */
char LetterSet[] = "##TEMNAIOGKDWRUS##QZYCXBJP#L#FVH09#8###7#####\\=61####+##2###3#45";

// baud settings
#define BAUD_DURATION               120                  //mSec default
#define TOUCH_THRESHOLD             0                   //how long to wait in uSec, before sampling the touch pin.
#define interbaud_duration          BAUD_DURATION * 1   //wait between dots and dashes
#define dit_duration                BAUD_DURATION * 1   //length of a dot
#define dah_duration                BAUD_DURATION * 3   //length of a dash
#define NearLineEnd                 60                  //number of characters before sending newline (to make serial easier to read)

//waits
#define FullWait                    BAUD_DURATION * 1 //Length between characters
#define WordWait                    BAUD_DURATION * 4   //Length between words

int WaitWait = FullWait;        //Character counter
boolean characterDone = true;
int letterCount = 0;            //Line end counter
uint8_t downtime = 0;           //Counts a dit or a dah
int ShiftNum = 0;               //Number to be shifted and added to depending on dits or dahs
int NewWord = WordWait;         //Word counter

void readDit()  //Checks if there's a dit
{
  delayMicroseconds(TOUCH_THRESHOLD);
  if (digitalRead(DIT_PIN))
  {
    dit = 1;
  }
  else
  {
    dit = 0;
  }
}

void readDah()  //Checks if there's a dah
{
  delayMicroseconds(TOUCH_THRESHOLD);
  if (digitalRead(DAH_PIN))
  {
    dah = 1;
  }
  else
  {
    dah = 0;
  }
}

void setup()
{
  // setup input pins
  pinMode(DIT_PIN, INPUT);
  pinMode(DAH_PIN, INPUT);

  // setup output pins
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW); //turn off led
  Serial.begin(9600);
  state = IDLE;
}

// handles the output of the ciruit.
// if state is 1 then the contact is closed or led is turned on
void contact(unsigned char state)
{
  if (state)
  {
    digitalWrite(LED, HIGH);
    tone(SPEAKER, 650);
  }
  else
  {
    digitalWrite(LED, LOW);
    noTone(SPEAKER);
  }
}

void loop()
{
  if (state == IDLE)  //If nothing is being pressed
  {
    if (NewWord > 0)
      NewWord--;  //Deplete new word counter
    if (NewWord == 1)
      printSpace(); //Print space if the counter is done (word over)

    // now, if dit is pressed go there, else check for dah
    readDit();
    if (dit)
    {
      state = DIT;
    }
    else
    {
      readDah();
      if (dah)
      {
        state = DAH;
      }
    }
    
    if (!characterDone)
    {
      WaitWait--; //Count down the character counter
      if (WaitWait == 0) //Character is done
      {
        printCharacter(); //Print it
        characterDone = true; //The character is done
        ShiftNum = 0; //Reset the shifting number
      }
    }
  }
  else if (state == DIT)
  {
    WaitWait = FullWait; //Reset the counters
    NewWord = WordWait;
    characterDone = false;
    
    contact(1);
    delay(dit_duration);
    contact(0);
    delay(interbaud_duration);
    downtime++;
    
    if (ShiftNum == 0)
    {
      ShiftNum = 1; // This is our start bit
    }
    shiftBits();
    
    // now, if dah is pressed go there, else check for dit
    readDah();
    if (dah)
    {
      state = DAH;
    }
    else
    {
      // read dit now
      readDit();
      if (dit)
      {
        state = DIT;
      }
      else
      {
        delay(interbaud_duration);
        state = IDLE;
      }
    }
  }
  else if (state == DAH)
  {
    WaitWait = FullWait; //Reset counters
    NewWord = WordWait;
    characterDone = false;
    
    contact(1);
    delay(dah_duration);
    contact(0);
    delay(interbaud_duration);
    downtime += 3;
    
    if (ShiftNum == 0)
    {
      ShiftNum = 1; // This is our start bit
    }
    shiftBits();
    
    // now, if dit is pressed go there, else check for dah
    readDit();
    if (dit)
    {
      state = DIT;
    }
    else
    {
      //read dit now
      readDah();
      if (dah)
      {
        state = DAH;
      }
      else
      {
        delay(interbaud_duration);
        state = IDLE;
      }
    }
  }
  delay(1);
}

void printSpace()
{
  letterCount++;
  if (letterCount > NearLineEnd)
  {
    Serial.println(); //Print newline if we're over 60 characters on the current line
    letterCount = 0;
    return; // Go back to loop(), we're done here.
  }
  Serial.print(' ');
}

void shiftBits()
{
  //Serial.println('m');
  if (downtime == 1)
  {
    ShiftNum = ShiftNum << 1; // shift bits left
    ShiftNum++; //Add one
  }
  else if (downtime == 3)
  {
    // We got a dah
    ShiftNum = ShiftNum << 1; // shift bits left
  }
  downtime = 0;
}

void printCharacter()
{
  NewWord = WordWait;
  letterCount++;
  if (ShiftNum > 63)  //Not in the array
  {
    printPunctuation();
    return; // Go back to the main loop(), we're done here.
  }
  Serial.print(LetterSet[ShiftNum]);
}

void printPunctuation()
{
  char pMark = '#'; // Just in case nothing matches
  if (ShiftNum == 71)
    pMark = ':';
  else if (ShiftNum == 76)
    pMark = ',';
  else if (ShiftNum == 85)
    pMark = ';';
  else if (ShiftNum == 84)
    pMark = '!';
  else if (ShiftNum == 94)
    pMark = '-';
  else if (ShiftNum == 97)
    pMark = '\'';
  else if (ShiftNum == 101)
    pMark = '@';
  else if (ShiftNum == 106)
    pMark = '.';
  else if (ShiftNum == 109)
    pMark = '"';
  else if (ShiftNum == 114)
    pMark = '_';
  else if (ShiftNum == 115)
    pMark = '?';
  Serial.print(pMark);
}
