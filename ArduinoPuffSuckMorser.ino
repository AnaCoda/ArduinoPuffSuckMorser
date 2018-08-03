/*
Based on tutorial at http://mypetarduino.com/ReadCode/readCode.01.html
and https://vk8da.com/an-arduino-iambic-keyer/
*/

// Iambic keyer
#define DIT_PIN 8
#define DAH_PIN 10
#define LED 13
#define BAUD_DURATION 150  //mSec default
#define TOUCH_THRESHOLD 0 // how long to wait in uSec, before sampling the touch pin.

// baud up/down pins
#define BAUDUP_PIN 7
#define BAUDDOWN_PIN 4

// keyer state
enum
{
  IDLE,
  DIT,
  DAH,
};

int dit, dah;
int state;

// baud settings
#define BAUD_MIN 10
#define BAUD_MAX 200

unsigned long baud = BAUD_DURATION;

unsigned long interbaud_duration = baud * 1;
unsigned long dit_duration = baud;
unsigned long dah_duration = baud * 3;

char mySet[] = "##TEMNAIOGKDWRUS##QZYCXBJP#L#FVH09#8###7#######61#######2###3#45";

long FullWait = BAUD_DURATION / 2;
long WaitWait = FullWait;

int nearLineEnd = 60;
int letterCount = 0;

boolean characterDone = true;

uint8_t downtime = 0;

int myNum = 0;

long newWord = FullWait * 10;

void readDit()
{
  delayMicroseconds(TOUCH_THRESHOLD);
  if (digitalRead(DIT_PIN))
  {
    dit = 0;
  }
  else
  {
    dit = 1;
  }
}

void readDah()
{
  delayMicroseconds(TOUCH_THRESHOLD);
  if (digitalRead(DAH_PIN))
  {
    dah = 0;
  }
  else
  {
    dah = 1;
  }
}

void setup()
{
  // setup input pins
  pinMode(DIT_PIN, INPUT_PULLUP);
  pinMode(DAH_PIN, INPUT_PULLUP);

  // setup output pins
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW); //turn off led
  Serial.begin(9600);
  state = 0;
}

// handles the output of the ciruit.
// if state is 1 then the contact is closed or led is turned on
void contact(unsigned char state)
{
  if (state)
  {
    digitalWrite(LED, HIGH);
  }
  else
  {
    digitalWrite(LED, LOW);
  }
}

void loop()
{
  if (state == IDLE)
  {
    if (newWord > 0)
      newWord--;
    if (newWord == 1)
      printSpace();
    readDit();
    if (dit)
    {
      state = DIT;
    }
    else
    {
      delayMicroseconds(0);
      readDah();
      if (dah)
      {
        state = DAH;
      }
    }
    if (!characterDone)
    {
      WaitWait--; //We are counting down
      if (WaitWait == 0)
      {
        printCharacter();
        characterDone = true;
        myNum = 0;
      }
    }
  }
  else if (state == 1)
  {
    WaitWait = FullWait;
    characterDone = false;
    contact(1);
    delay(dit_duration);
    contact(0);
    delay(interbaud_duration);
    downtime++;
    if (myNum == 0)
    {
      myNum = 1; // This is our start bit
    }
    printMorse();
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
    //Serial.println("dah");
    WaitWait = FullWait;
    characterDone = false;
    contact(1);
    delay(dah_duration);
    contact(0);
    delay(interbaud_duration);
    downtime += 3;
    if (myNum == 0)
    {
      myNum = 1; // This is our start bit
    }
    printMorse();
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
  } // switch
  delay(1);
}

void printSpace()
{
  letterCount++;
  if (letterCount > nearLineEnd)
  {
    Serial.println();
    letterCount = 0;
    return; // Go back to loop(), we're done here.
  }
  Serial.print(' ');
}

void printMorse()
{
  //Serial.println('m');
  if (downtime == 1)
  {
    // We got a dit
    myNum = myNum << 1; //shift bits left
    myNum++;
  }
  else if (downtime == 3)
  {
    // We got a dah
    myNum = myNum << 1; // shift bits left
  }
  downtime = 0;
}

void printCharacter()
{
  newWord = FullWait * 10;
  letterCount++;
  if (myNum > 63)
  {
    printPunctuation();
    return; // Go back to the main loop(), we're done here.
  }
  Serial.print(mySet[myNum]);
}

void printPunctuation()
{
  char pMark = '#'; // Just in case nothing matches
  if (myNum == 71)
    pMark = ':';
  if (myNum == 76)
    pMark = ',';
  if (myNum == 84)
    pMark = '!';
  if (myNum == 94)
    pMark = '-';
  if (myNum == 101)
    pMark = '@';
  if (myNum == 106)
    pMark = '.';
  if (myNum == 115)
    pMark = '?';
  Serial.print(pMark);
}
