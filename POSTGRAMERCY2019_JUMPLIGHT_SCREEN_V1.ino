//Post Grammercy 2019 version for Jump into the Light

//removed added time before Always Away

//for 30x30 standing matrix
//KGRAMERCY 2019 version
//BEAUTIFUL and RADIO GAGA are new tracks from Leo
//written by David Crittenden 7/2018
//updated 3/2019
//BUTTON LAYOUT
/*
  row 1: ALWAYS AWAY / EVERYTHING IS BREAUTIFUL / INFERNO / ALL AROUND YOU
  row 2: RADIO GAGA / BEAUTIFUL / SOMETHING / SOMETHING eLSE
  row 3: CRISTIANO DANCE MUSIC / HEART FRAME FADE / Circle Droplet / Multi - Color Shock Wave
  row 4: FUCK TRUMP / Rainbow Checkers / Red-White Heart / CLEAR ALL
*/

//timing with millis()
//Functional with dma zero library!
//https://forum.arduino.cc/index.php?topic=476630.0 to explain how to resolve conflicts with the TC3 handler

//uses the zero dma libraries https://learn.adafruit.com/dma-driven-neopixels/overview
//code derived from https://learn.adafruit.com/remote-effects-trigger/overview-1?view=all
//see also https://learn.adafruit.com/adafruit-feather-32u4-radio-with-rfm69hcw-module/using-the-rfm69-radio
//code derived from https://github.com/adafruit/CircuitPlayground_Power_Reactor
//Ada_remoteFXTrigger_RX_NeoPixel
//Remote Effects Trigger Box Receiver
//by John Park
//for Adafruit Industries
//MIT License
//Modified by David Crittenden 11/2017 - 3/2019

#include "lerp.h"//deals with linear interpolation
#include "bitmapData.h"//stores bitmap data
#include <Adafruit_GFX.h>
#include <avr/pgmspace.h> //for the bitmap in program memory
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <Adafruit_NeoMatrix_ZeroDMA.h>//https://learn.adafruit.com/dma-driven-neopixels/overview
#include <SPI.h>
#include <RH_RF69.h>
#include <Wire.h>

#define LED 13

/********** NeoPixel Setup *************/
#define PIN            11

Adafruit_NeoMatrix_ZeroDMA matrix(30, 30, PIN,
                                  NEO_MATRIX_BOTTOM     + NEO_MATRIX_LEFT +
                                  NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
                                  NEO_GRB            + NEO_KHZ800);

/************ Radio Setup ***************/
// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

//#if defined(ARDUINO_SAMD_FEATHER_M0) // Feather M0 w/Radio
#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
//#endif

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

bool oldState = HIGH;
int showType = 0;

////////////////////////////variables used in different functions
//radio variables
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);

//used for mode cycling
uint8_t mode = 0;
uint8_t currentMode;
uint8_t previousMode = 0;

//used for timing
int pass = 0;
int previousPass;
int count = 1;
int previousCount;
int segment = 0;//counter for parts in case loops
int previousSegment;
int i = 0;//ugh....
int part;//counter for loops within functions
bool ready = true;
int speed;

int bpm;
unsigned long oneBeat;//calculates the number of milliseconds of one beat
unsigned long oneBar;//calculates the length of one bar in milliseconds
unsigned long currentMillis;
unsigned long startMillis;
unsigned long reStartMillis;
unsigned long previousRestartMillis;
unsigned long previousMillis;
unsigned long elapsedMillis;
unsigned long previousElapsedMillis;

//used for color cycling
uint8_t R;
uint8_t G;
uint8_t B;
uint8_t RA;
uint8_t GA;
uint8_t BA;
uint8_t red;
uint8_t green;
uint8_t blue;

int x = matrix.width();
int previousX;
int y = matrix.height();
int previousY;

//used for multiple random locations
byte XA = random(matrix.width());
byte YA = random(matrix.height());
byte XB = random(matrix.width());
byte YB = random(matrix.height());
byte XC = random(matrix.width());
byte YC = random(matrix.height());

//used for blacking out rectangles
int blackXA = random(matrix.width());
int blackYA = random(matrix.height());
int blackX = random(-6, matrix.width());
int blackY = random(-6, matrix.height());

//used for randomizing locations
int X;
int Y;

uint8_t side = 2;//boxMarch()
uint8_t j = 0;//color starts at 0 used in rainbow and wheel functions

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  delay(500);
  Serial.begin(115200);
  /*while (!Serial)
    {
    delay(1);  // wait until serial console is open, remove if not tethered to computer
    }*/

  matrix.begin();
  matrix.setBrightness(255);
  matrix.show();

  randomSeed(analogRead(0));
  //fill in all the random variables with starting values
  getRandom();
  getCircleRandom();
  getRectangleRandom();
  getBlackRandom();

  pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 RX/TX Test!");
  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  if (!rf69.init())
  {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ))
  {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(14, true);//set to 20 on transmitter

  // The encryption key has to be the same as the one in the server
  uint8_t key[] =
  {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
  };
  rf69.setEncryptionKey(key);

  pinMode(LED, OUTPUT);//sets the onboard led
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  //splash screen
  matrix.fillScreen(matrix.Color(255, 0, 0));
  matrix.drawBitmap(0, 0, sunOne, 30, 30, (matrix.Color(255, 255, 255)));
  matrix.show();
  delay(2000);
  matrix.fillScreen(0);
  matrix.show();

  zeroOut();
  timeCheck();
}


void loop()
{
  buttonCheck();

  switch (mode)
  {
    case 0:
      matrix.fillScreen(0);
      matrix.show();
      buttonCheck();
      break;

    case 1://ALWAYS AWAY - removed 98 sec count in - 120 BPM
      bpm = 120;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;
      /*
            //ADDED BEATS OF SILENCE FOR COUNT IN
            if (ready == true)
            {
              segment = 0;
              ready = false;
            }
            while (elapsedMillis < 98000 && segment == 0)//COUNT IN
            {
              buttonCheck();
              if (currentMode != mode) break;
              timeCheck();
            }
            if (elapsedMillis >= (oneBar) && segment == 0) segment = 1, count = 1;*/

      while (elapsedMillis < 98000 && segment == 1)//INTRO
      {
        buttonCheck();
        if (currentMode != mode) break;

        randomRectangle(2, 3, false, false, 1, 0);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 2)//CHORUS C1
      {
        buttonCheck();
        if (currentMode != mode) break;

        R = random(255);
        G = random(255);
        B = 0;
        randomRectangle(0, 0, true, true, 1 , oneBeat);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 3)//VERSE A1
      {
        buttonCheck();
        if (currentMode != mode) break;

        boxPin(1, 0, false, oneBeat / 2);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 4)//CHORUS C2
      {
        buttonCheck();
        if (currentMode != mode) break;

        R = 0;
        G = random(255);
        B = random(255);
        randomRectangle(0, 0, true, true, 1 , oneBeat);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 5)//MORPH
      {
        buttonCheck();
        if (currentMode != mode) break;

        RA = 127;
        GA = 0;
        BA = 0;
        X = matrix.width() / 2;
        Y = matrix.height() - 1;
        sineDroplet(1, false, 1.4 , matrix.height() + 5, 0, 12);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 6)//INTRO REPRISE
      {
        buttonCheck();
        if (currentMode != mode) break;

        randomRectangle(2, 4, false, false, 1, 0);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 7)//CHORUS C3
      {
        buttonCheck();
        if (currentMode != mode) break;

        R = random(255);
        G = 0;
        B = random(255);
        randomRectangle(0, 0, true, true, 1 , oneBeat);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//MORPH REPRISE
      {
        buttonCheck();
        if (currentMode != mode) break;

        RA = 255;
        GA = 255;
        BA = 0;
        X = matrix.width() / 2;
        Y = matrix.height() - 1;
        sineDroplet(1, false, 1.4 , matrix.height() + 5, 0, 12);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//VERSE A2
      {
        buttonCheck();
        if (currentMode != mode) break;

        boxPin(1, 0, false, oneBeat / 2);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 10)//CHORUS C4
      {
        buttonCheck();
        if (currentMode != mode) break;

        R = 0;
        G = random(255);
        B = random(255);
        randomRectangle(0, 0, true, true, 1 , oneBeat);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//CHORUS C5
      {
        buttonCheck();
        if (currentMode != mode) break;

        j = j + 8;
        if (j > 255) j = 0;
        randomRectangle(2, 0, true, true, 1 , oneBeat);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 7) && segment == 12)//SOLO
      {
        buttonCheck();
        if (currentMode != mode) break;

        RA = 0;
        GA = 0;
        BA = 0;
        sineDroplet(1, false, 0.4 , 5, 0, 30);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 7) && segment == 12) segment = 13, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 1) && segment == 13)//INTRO REPRISE HARD END
      {
        buttonCheck();
        if (currentMode != mode) break;

        randomRectangle(2, 5, false, false, 2 , 0);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 1) && segment == 13) segment = 14, count = 1;
      //HARD END
      matrix.fillScreen(0);
      matrix.show();
      break;

    case 2://EVERYTHING IS BEAUTIFUL UNDER THE SUN - NO COUNT IN - 129 BPM
      bpm = 129;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      while (elapsedMillis < (oneBar) && segment == 1)//INTRO
      {
        buttonCheck();
        if (currentMode != mode) break;
        R = 0;
        G = 0;
        B = 255;
        fadeUp(0, 2, (oneBeat * 17));

        timeCheck();
      }

      if (elapsedMillis >= (oneBar) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 8) - (oneBeat * 4)) && segment == 2)//VERSE A1
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;

          circleDroplet(true, oneBeat * 4);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 3))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 0;
          B = 40;
          RA = 0;
          GA = 0;
          BA = 0;
          heartbeat(0, 3, oneBeat);

          timeCheck();
        }
        matrix.fillScreen(matrix.Color(255, 0, 0));
        while (elapsedMillis >= (oneBar * 3) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = random(80);
          G = 0;
          B = 0;
          pixelSprinkle(0, 2, 5, 20);

          timeCheck();
        }
        getCircleRandom();
        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;

          circleDroplet(true, oneBeat * 4);

          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar * 6) && elapsedMillis < (oneBar * 8) - (oneBeat * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 0;
          B = 0;
          RA = 255;
          GA = 255;
          BA = 0;
          sineDroplet(0, true, 0.9, 25, 1, 100);

          timeCheck();
        }
        timeCheck();
      }

      if (elapsedMillis >= ((oneBar * 8) - (oneBeat * 4)) && segment == 2) segment = 3, count = 5;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 8) + (oneBeat * 4)) && segment == 3)//CHORUS C1
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBeat * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          everythingScroll(1, 2, 20);

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 4) && elapsedMillis <  (oneBeat * 10))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 20);

          timeCheck();
        }
        i = 0;//reset sunrise placement
        while (elapsedMillis >= (oneBeat * 10) && elapsedMillis <  (oneBeat * 16))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 255;
          fadeUp(0, 2, (oneBeat * 7));

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 16) && elapsedMillis < (oneBeat * 20))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          everythingScroll(1, 2, 20);

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 20) && elapsedMillis <  (oneBeat * 26))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 20);

          timeCheck();
        }
        i = 0;//reset sunrise placement
        while (elapsedMillis >= (oneBeat * 26) && elapsedMillis <  (oneBeat * 32))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 255;
          fadeUp(0, 2, (oneBeat * 7));

          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBeat * 32) && elapsedMillis < ((oneBar * 8) + (oneBeat * 4)))
        {
          buttonCheck();
          if (currentMode != mode) break;

          boxPin(2, 1, false, 250);

          timeCheck();
        }
        timeCheck();
      }

      if (elapsedMillis >= ((oneBar * 8) + (oneBeat * 4)) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      getCircleRandom();
      while (elapsedMillis < ((oneBar * 8) - (oneBeat * 4)) && segment == 4)//VERSE A2
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;

          circleDroplet(true, oneBeat * 4);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 3))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 0;
          B = 40;
          RA = 0;
          GA = 0;
          BA = 0;
          heartbeat(0, 3, oneBeat);

          timeCheck();
        }
        matrix.fillScreen(matrix.Color(255, 0, 0));
        while (elapsedMillis >= (oneBar * 3) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = random(80);
          G = 0;
          B = 0;
          pixelSprinkle(0, 2, 5, 20);

          timeCheck();
        }
        getCircleRandom();
        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;

          circleDroplet(true, oneBeat * 4);

          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar * 6) && elapsedMillis < (oneBar * 8) - (oneBeat * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 0;
          B = 0;
          RA = 255;
          GA = 255;
          BA = 0;
          sineDroplet(0, true, 0.9, 25, 1, 100);

          timeCheck();
        }
      }

      if (elapsedMillis >= ((oneBar * 8) - (oneBeat * 4)) && segment == 4) segment = 5, count = 5;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 12) + (oneBeat * 4)) && segment == 5)//CHORUS C2
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBeat * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          everythingScroll(1, 2, 20);

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 4) && elapsedMillis <  (oneBeat * 10))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 20);

          timeCheck();
        }
        i = 0;//reset sunrise placement
        while (elapsedMillis >= (oneBeat * 10) && elapsedMillis <  (oneBeat * 16))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 255;
          fadeUp(0, 2, (oneBeat * 7));

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 16) && elapsedMillis < (oneBeat * 20))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          everythingScroll(1, 2, 20);

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 20) && elapsedMillis <  (oneBeat * 26))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 20);

          timeCheck();
        }
        i = 0;//reset sunrise placement
        while (elapsedMillis >= (oneBeat * 26) && elapsedMillis <  (oneBeat * 32))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 255;
          fadeUp(0, 2, (oneBeat * 7));

          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBeat * 32) && elapsedMillis < ((oneBar * 12) + (oneBeat * 4)))
        {
          buttonCheck();
          if (currentMode != mode) break;

          boxPin(2, 1, false, 250);

          timeCheck();
        }
        timeCheck();
      }

      if (elapsedMillis >= ((oneBar * 12) + (oneBeat * 4)) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      matrix.fillScreen(matrix.Color(255, 0, 255));
      while (elapsedMillis < (oneBar * 8) && segment == 6)//BREAK
      {
        buttonCheck();
        if (currentMode != mode) break;
        while (elapsedMillis < (oneBar))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 0;
          pixelSprinkle(0, 2, 3, 20);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar) && elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 255;
          pixelSprinkle(0, 2, 3, 20);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 3))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 255;
          B = 0;
          pixelSprinkle(0, 2, 3, 20);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 3) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          pixelSprinkle(2, 2, 3, 20);

          timeCheck();
        }

        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 0;
          B = 127;
          RA = 60;
          GA = 0;
          BA = 0;
          heartbeat(0, 2, oneBeat);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 6) && elapsedMillis < (oneBar * 8))
        {
          buttonCheck();
          if (currentMode != mode) break;

          boxPin(2, 2, false, oneBeat / 2);

          timeCheck();
        }
        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 8) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 4) - (oneBeat * 4)) && segment == 7)//INTERLUDE
      {
        buttonCheck();
        if (currentMode != mode) break;

        R = 255;
        G = 0;
        B = 0;
        RA = 255;
        GA = 0;
        BA = 255;
        sineLarson(false, 4, 2.5);

        timeCheck();
      }

      if (elapsedMillis >= ((oneBar * 4) - (oneBeat * 4)) && segment == 7) segment = 8, count = 5;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 10) + (oneBeat * 4)) && segment == 8)//CHORUS C3
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBeat * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          everythingScroll(1, 2, 20);

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 4) && elapsedMillis <  (oneBeat * 10))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 20);

          timeCheck();
        }
        i = 0;//reset sunrise placement
        while (elapsedMillis >= (oneBeat * 10) && elapsedMillis <  (oneBeat * 16))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 255;
          fadeUp(0, 2, (oneBeat * 6));

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 16) && elapsedMillis < (oneBeat * 20))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          everythingScroll(1, 2, 20);

          timeCheck();
        }
        x = matrix.width();
        while (elapsedMillis >= (oneBeat * 20) && elapsedMillis <  (oneBeat * 26))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 20);

          timeCheck();
        }
        i = 0;//reset sunrise placement
        while (elapsedMillis >= (oneBeat * 26) && elapsedMillis <  (oneBeat * 32))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 255;
          fadeUp(0, 2, (oneBeat * 6));

          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBeat * 32) && elapsedMillis < ((oneBar * 10) + (oneBeat * 4)))
        {
          buttonCheck();
          if (currentMode != mode) break;

          boxPin(2, 1, false, 250);

          timeCheck();
        }
        timeCheck();
      }

      if (elapsedMillis >= ((oneBar * 10) + (oneBeat * 4)) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 3) && segment == 9)//INTRO REPRISE
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBar + (oneBeat * 4)))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 255;
          fadeUp(0, 2, (oneBeat * 12));

          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar + (oneBeat * 4)) && elapsedMillis < (oneBar * 3))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 0;
          B = 0;
          RA = 255;
          GA = 0;
          BA = 255;
          sineLarson(false, 4, .15);

          timeCheck();
        }
        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 3) && segment == 9) segment = 10, count = 1;

      matrix.fillScreen(0);
      matrix.show();
      break;

    case 3://INFERNO - NO COUNT IN - 134-135 BPM *varies to match the file 4:11
      bpm = 134;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;
      //NO COUNT IN
      while (elapsedMillis < (oneBar * 2) && segment == 1)//INTRO
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = random(100, 255);
        G = random(60);
        B = random(60);
        randomRectangle(1, 3, true, true, 0, 300);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 2)//VERSE A1a
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        randomPlaid(2, 4, true, true, 1, oneBeat / 2);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 3)//VERSE A1b
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        boxPin(true, false, true, oneBeat / 2);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      //adjust for timing here
      bpm = 135;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      while (elapsedMillis < (oneBar * 4) && segment == 4)//VERSE B1a
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 160;
        G = 0;
        B = 20;
        RA = 255;
        GA = 125;
        BA = 0;
        rotatingSquare(3, false, true, 0, true, oneBeat * 2);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 5)//VERSE B1b
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 160;
        G = 0;
        B = 20;
        RA = 255;
        GA = 125;
        BA = 0;
        rotatingSquare(3, false, true, 0, false, oneBeat * 2);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 6)//CHORUS C1a
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 100;
        G = 0;
        B = 20;
        RA = 255;
        GA = 180;
        BA = 0;
        sineLarson(false, 7, 1.8);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 7)//CHORUS C1b
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = random(100, 255);
        G = random(50);
        B = random(50);
        speed = map(elapsedMillis, 0, (oneBar * 4), 500, 5);
        randomTriangle(0, 1, true, speed);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//MORPH1
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 90;
        G = 0;
        B = 30;
        RA = 180;
        GA = 0;
        BA = 255;
        sineDroplet(0, true, 0.3, 25, 2, 0);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//VERSE A2a
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        randomPlaid(2, 4, true, true, 1, oneBeat / 2);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 10)//VERSE A2b
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        boxPin(true, false, true, oneBeat / 2);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//VERSE B2a
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 160;
        G = 0;
        B = 20;
        RA = 255;
        GA = 125;
        BA = 0;
        rotatingSquare(3, false, true, 0, true, oneBeat * 2);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 12)//VERSE B2b
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 160;
        G = 0;
        B = 20;
        RA = 255;
        GA = 125;
        BA = 0;
        rotatingSquare(3, false, true, 0, false, oneBeat * 2);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 12) segment = 13, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 13)//CHORUS C2a
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 100;
        G = 0;
        B = 20;
        RA = 255;
        GA = 180;
        BA = 0;
        sineLarson(false, 7, 1.8);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 13) segment = 14, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 14)//CHORUS C2b
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = random(100, 255);
        G = random(50);
        B = random(50);
        speed = map(elapsedMillis, 0, (oneBar * 4), 500, 5);
        randomTriangle(0, 1, true, speed);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 14) segment = 15, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 15)//MORPH2
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 90;
        G = 0;
        B = 30;
        RA = 180;
        GA = 0;
        BA = 255;
        sineDroplet(0, true, 0.3, 25, 2, 0);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 15) segment = 16, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 16)//CHORUS C3a
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 100;
        G = 0;
        B = 20;
        RA = 255;
        GA = 180;
        BA = 0;
        sineLarson(false, 7, 1.8);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 16) segment = 17, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 17)//CHORUS C3b
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = random(100, 255);
        G = random(50);
        B = random(50);
        speed = map(elapsedMillis, 0, (oneBar * 4), 500, 5);
        randomTriangle(0, 1, true, speed);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 17) segment = 18, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 18)//MORPH3
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 90;
        G = 0;
        B = 30;
        RA = 180;
        GA = 0;
        BA = 255;
        sineDroplet(0, true, 0.3, 25, 2, 0);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 18) segment = 19, count = 1;

      segmentZero(false);
      while (elapsedMillis < (oneBeat * 4) && segment == 19)//SOFT END
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = 0;
        G = 0;
        B = 0;
        pixelSprinkle(0, 2, 10, 0);

        timeCheck();
      }
      if (elapsedMillis >= (oneBeat * 4) && segment == 19) segment = 20, count = 1;
      matrix.fillScreen(0);
      matrix.show();
      break;

    case 4://ALL AROUND YOU - REMOVED WITH 8 BEAT COUNT IN - 120 BPM 4:12
      bpm = 120;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      /*//ADDED BEATS OF SILENCE FOR COUNT IN
        if (ready == true)
        {
        segment = 0;
        ready = false;
        }
        while (elapsedMillis < (oneBar) && segment == 0)//COUNT IN
        {
        buttonCheck();
        if (currentMode != mode) break;
        timeCheck();
        }*/
      if (elapsedMillis >= (oneBar) && segment == 0) segment = 1, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar) && segment == 1)//INTRO
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        R = random(255);
        G = 0;
        B = 0;
        pixelSprinkle(0, 0, 4, 10);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 1) segment = 2, count = 1;

      segmentZero(false);
      while (elapsedMillis < (oneBar * 6) && segment == 2)//VERSE A1
      {
        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;
          //do this
          R = map(pass, matrix.width() / 2, 1, 0, 255);
          G = map(pass, matrix.width() / 2, 1, 0, 255);
          B = map(pass, matrix.width() / 2, 1, 155, 0);

          boxMarch(false, false, true, oneBar);
          timeCheck();
        }
        while (elapsedMillis >= oneBar * 2 && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;
          //do
          R = 0;
          G = 150;
          B = 100;
          RA = 0;
          GA = 0;
          BA = 255;
          sineLarson(false, 1, 0.9);//with scrolling cloud frame

          currentMillis = millis();
          if (currentMillis - previousMillis >= 80)
          {
            previousMillis = currentMillis;
            y++;//advance the scroll
          }
          if (y >= matrix.height()) y = -41;

          timeCheck();
        }
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 3)//VERSE A2
      {
        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 255;
          G = 232;
          B = 61;
          RA = 173;
          GA = 17;
          BA = 17;
          X = matrix.width() / 2;
          Y = matrix.height() / 2;
          circleDroplet(false, 1900);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;
          circleDroplet(true, 950);
          timeCheck();
        }
      }

      if (elapsedMillis >= (oneBar * 6) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 4)//FIRST CHORUS
      {
        //do this
        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 255;
          G = 255;
          B = 0;
          RA = 100;
          GA = 0;
          BA = 255;
          //rotatingSquare(uint8_t multi, bool frame, bool sine, bool variant, bool blackOut, unsigned long interval);
          rotatingSquare(3, false, true, false, true, 465);
          timeCheck();
        }

        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;
          allAroundScroll(false, 0, 22);
          timeCheck();
        }

        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 0;
          G = 255;
          B = 0;
          RA = 100;
          GA =  0;
          BA = 100;
          sineLarson(false, 2, 0.7);// with NEED

          timeCheck();
        }
      }

      if (elapsedMillis >= (oneBar * 6) && segment == 4) segment = 5, count = 1;

      segmentZero(false);
      while (elapsedMillis < (oneBar * 6) && segment == 5)//VERSE B1
      {
        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;
          pixelSprinkle(1, 0, 0, 5);
          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;
          boxPin(1, 0, true, (oneBeat));
          timeCheck();
        }
      }

      if (elapsedMillis >= (oneBar * 6) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 6)//VERSE B2
      {
        buttonCheck();
        if (currentMode != mode) break;
        boxMarch(true, false, false, (oneBeat * 2));
        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 6) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 8) && segment == 7)//CHORUS C2
      {
        //do this
        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 255;
          G = 255;
          B = 0;
          RA = 0;
          GA = 80;
          BA = 90;
          //rotatingSquare(uint8_t multi, bool frame, bool sine, bool variant, bool blackOut, unsigned long interval);
          rotatingSquare(3, false, true, false, false, 465);
          timeCheck();
        }

        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;
          allAroundScroll(false, 0, 22);
          timeCheck();
        }

        part = 0;
        x = matrix.width();
        matrix.fillScreen(0);

        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 0;
          G = 255;
          B = 255;
          RA = 100;
          GA = 0;
          BA = 0;
          //rotatingSquare(uint8_t multi, bool frame, bool sine, bool variant, bool blackOut, unsigned long interval);
          rotatingSquare(3, false, true, false, false, 465);
          timeCheck();
        }

        while (elapsedMillis >= (oneBar * 6) && elapsedMillis < (oneBar * 8))
        {
          buttonCheck();
          if (currentMode != mode) break;
          allAroundScroll(false, 0, 22);
          timeCheck();
        }
      }

      if (elapsedMillis >= (oneBar * 8) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//MORPH
      {
        //do this
        while (elapsedMillis < oneBar)
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 255;
          G = 0;
          B = 0;
          RA = 0;
          GA = 0;
          BA = 0;
          heartbeat(0, 0, oneBeat);
          timeCheck();
        }

        while (elapsedMillis >= oneBar && elapsedMillis < oneBar * 2)
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 0;
          G = 255;
          B = 0;
          RA = 0;
          GA = 0;
          BA = 0;
          heartbeat(0, 0, oneBeat);
          timeCheck();
        }

        while (elapsedMillis >= oneBar * 2 && elapsedMillis < oneBar * 3)
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 0;
          G = 0;
          B = 255;
          RA = 0;
          GA = 0;
          BA = 0;
          heartbeat(0, 0, oneBeat);
          timeCheck();
        }

        while (elapsedMillis >= oneBar * 3  && elapsedMillis < oneBar * 4)
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 255;
          G = 255;
          B = 255;
          RA = 0;
          GA = 0;
          BA = 0;
          heartbeat(0, 0, oneBeat / 2);
          timeCheck();
        }
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//THIRD CHORUS
      {
        //do this
        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;
          allAroundScroll(false, 1, 22);
          timeCheck();
        }

        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 255;
          G = 255;
          B = 0;
          RA = 40;
          GA = 0;
          BA = 90;
          //rotatingSquare(bool multi, bool frame, bool sine, bool variant, bool blackOut, unsigned long interval);
          rotatingSquare(false, false, true, false, true, oneBeat);
          timeCheck();
        }
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 10)//GUITAR SOLO PART 1
      {
        buttonCheck();
        if (currentMode != mode) break;

        /* R = 0;
          G = 255;
          B = 0;
          RA = 100;
          GA = 0;
          BA = 100;*/
        sineDroplet(2, false, 0.9, 25, 0, (oneBeat / 27));
        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 6) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//GUITAR SOLO PART 2
      {
        buttonCheck();
        if (currentMode != mode) break;
        sineDroplet(1, false, 0.9, 25, 0, (oneBeat / 27));
        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 12)//FINAL CHORUS
      {
        while (elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;
          allAroundScroll(false, 2, 22);
          timeCheck();
        }

        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = 255;
          G = 0;
          B = 255;
          RA = 40;
          GA = 50;
          BA = 0;
          //rotatingSquare(bool multi, bool frame, bool sine, bool variant, bool blackOut, unsigned long interval);
          rotatingSquare(false, false, true, false, true, oneBeat * 4);
          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;
          R = random(127);
          G = 0;
          B = random(255);
          pixelSprinkle(0, 0, 10, 20);
          timeCheck();
        }
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 12) segment = 13, count = 1;

      matrix.fillScreen(0);
      matrix.show();
      break;

    case 5://Radio Ga Ga 113 BPM
      bpm = 113;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      while (elapsedMillis < ((oneBar * 4) + (oneBeat * 4)) && segment == 1) //INTRO A
      {
        buttonCheck();
        if (currentMode != mode) break;

        randomTriangle(2, 0, false, 700);

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 4) + (oneBeat * 4)) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 4) + (oneBeat * 4)) && segment == 2)//INTRO B
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          matrixRainbow(4, 4, 250);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < ((oneBar * 4) + (oneBeat * 4)))
        {
          buttonCheck();
          if (currentMode != mode) break;

          matrixRainbow(7, 10, 80);

          timeCheck();
        }
        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 4) + (oneBeat * 4)) && segment == 2) segment = 3, count = 1;


      segmentZero(true);
      while (elapsedMillis < ((oneBar * 4) + (oneBeat * 4)) && segment == 3)//VERSE A1
      {
        buttonCheck();
        if (currentMode != mode) break;

        matrixRainbow(2, 2, 150);

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 4) + (oneBeat * 4)) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 4) + (oneBeat * 4)) && segment == 4)//VERSE A2
      {
        buttonCheck();
        if (currentMode != mode) break;

        matrixRainbow(3, 4, 100);

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 4) + (oneBeat * 4)) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 4) + (oneBeat * 4)) && segment == 5)//VERSE A3
      {
        buttonCheck();
        if (currentMode != mode) break;

        matrixRainbow(2, 2, 150);

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 4) + (oneBeat * 4)) && segment == 5) segment = 6, count = 1;


      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 6)//VERSE B1
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          randomTriangle(0, 0, true, 900);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;

          boxMarch(2, 2, false, oneBeat);

          timeCheck();
        }
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar) && segment == 7)//RADIO
      {
        buttonCheck();
        if (currentMode != mode) break;

        matrixRainbow(7, 10, 80);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 3) + (oneBeat * 4)) && segment == 8)//CHORUS C1 PART A
      {
        buttonCheck();
        if (currentMode != mode) break;

        radioScroll(2, 4, 80);

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 3) + (oneBeat * 4)) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 3) + (oneBeat * 4)) && segment == 9)//CHORUS C1 PART B
      {
        buttonCheck();
        if (currentMode != mode) break;

        R = 255;
        G = 255;
        B = 255;
        rotatingSquare(0, true, false, 2, false, (oneBeat * 2));

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 3) + (oneBeat * 4)) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 2) && segment == 10)//INTERLUDE
      {
        buttonCheck();
        if (currentMode != mode) break;

        randomTriangle(2, 0, false, 600);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 4) + (oneBeat * 4)) && segment == 11)//VERSE A3
      {
        buttonCheck();
        if (currentMode != mode) break;

        matrixRainbow(4, 6, 75);

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 4) + (oneBeat * 4)) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 12)//VERSE B2
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          randomTriangle(0, 0, true, 700);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 6))
        {
          buttonCheck();
          if (currentMode != mode) break;

          boxMarch(2, 2, false, oneBeat);

          timeCheck();
        }
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 12) segment = 13, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar) && segment == 13)//RADIO
      {
        buttonCheck();
        if (currentMode != mode) break;

        matrixRainbow(7, 10, 80);

        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 13) segment = 14, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 5) + (oneBeat * 4)) && segment == 14)//CHORUS C2 PART A
      {
        buttonCheck();
        if (currentMode != mode) break;

        radioSecondScroll(2, 6, 80);

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 5) + (oneBeat * 4)) && segment == 14) segment = 15, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 2) + (oneBeat * 4)) && segment == 15)//CHORUS C2 PART B
      {
        buttonCheck();
        if (currentMode != mode) break;

        R = 255;
        G = 255;
        B = 255;
        rotatingSquare(0, true, false, 2, false, (oneBeat * 2));

        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 2) + (oneBeat * 4)) && segment == 15) segment = 16, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 5) && segment == 16)//FINALE
      {
        buttonCheck();
        if (currentMode != mode) break;

        while (elapsedMillis < (oneBar))
        {
          buttonCheck();
          if (currentMode != mode) break;

          strobe(2, 1, 110);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar) && elapsedMillis < (oneBar * 2))
        {
          buttonCheck();
          if (currentMode != mode) break;

          strobe(1, 1, 110);

          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          strobe(0, 1, 110);

          timeCheck();
        }
        matrix.fillScreen(matrix.Color(255, 255, 255));
        while (elapsedMillis >= (oneBar * 4) && elapsedMillis < (oneBar * 5))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 0;
          G = 0;
          B = 0;
          pixelSprinkle(0, 0, 16, 20);

          timeCheck();
        }
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 5) && segment == 16) segment = 0, count = 1;

      matrix.fillScreen(0);
      matrix.show();
      break;

    case 6://BEAUTIFUL - NO COUNT IN - 115 BPM
      bpm = 115;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      while (elapsedMillis < (oneBar * 5) && segment == 1)//INTRO
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        randomTriangle(2, 0, false, 400);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 5) && segment == 1) segment = 2, count = 1;

      segmentZero(false);
      while (elapsedMillis < (oneBar * 4) && segment == 2)//VERSE A1
      {
        //do this
        while (elapsedMillis < (oneBar))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 28);
          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar) && elapsedMillis < (oneBar * 3))
        {
          buttonCheck();
          if (currentMode != mode) break;

          sineDroplet(2, false, 0.9, 8, 0, 30);
          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 3) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 0;
          RA = 255;
          GA = 40;
          BA = 0;
          sineLarson(false, 3, 1.9);
          timeCheck();

          currentMillis = millis();
          if (currentMillis - previousMillis >= (62))//speed of sunrise scroll
          {
            previousMillis = currentMillis;
            y--;
            if (y <= -30) y = matrix.height();
          }
        }
        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 2) segment = 3, count = 1;

      segmentZero(false);
      while (elapsedMillis < (oneBar * 4) && segment == 3)//VERSE A2
      {
        //do this
        while (elapsedMillis < (oneBar))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 28);
          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar) && elapsedMillis < (oneBar * 3))
        {
          buttonCheck();
          if (currentMode != mode) break;

          sineDroplet(1, false, 1.9, 8, 0, 30);
          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar * 3) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          pixelSprinkle(0, 0, 0, 2);

          timeCheck();
        }
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 4) + (oneBeat * 4)) && segment == 4) //CHORUS C1
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        while (elapsedMillis < (oneBar * 2))
        {
          randomTriangle(1, 0, true, 1200);
          timeCheck();
        }
        while (elapsedMillis >= ((oneBar * 4) + (oneBeat * 4)) && elapsedMillis < (oneBar * 4))
        {
          boxPin(1, 0, true, 400);
          timeCheck();
        }

        timeCheck();
      }

      if (elapsedMillis >= ((oneBar * 4) + (oneBeat * 4)) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 5)//VERSE A3
      {
        //do this
        while (elapsedMillis < (oneBar))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 28);
          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar) && elapsedMillis < (oneBar * 3))
        {
          buttonCheck();
          if (currentMode != mode) break;

          sineDroplet(1, false, 1.9, 8, 0, 30);
          timeCheck();
        }
        matrix.fillScreen(matrix.Color(60, 0, 0));
        while (elapsedMillis >= (oneBar * 3) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          pixelSprinkle(2, 2, 0, 0);

          timeCheck();
        }
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 5) segment = 6, count = 1;

      segmentZero(false);
      while (elapsedMillis < (oneBar * 4) && segment == 6)//VERSE A4
      {
        //do this
        while (elapsedMillis < (oneBar))
        {
          buttonCheck();
          if (currentMode != mode) break;

          beautifulScroll(2, 0, 5, 28);
          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar) && elapsedMillis < (oneBar * 3))
        {
          buttonCheck();
          if (currentMode != mode) break;

          sineDroplet(1, false, 1.9, 8, 0, 30);
          timeCheck();
        }
        matrix.fillScreen(0);
        while (elapsedMillis >= (oneBar * 3) && elapsedMillis < (oneBar * 4))
        {
          buttonCheck();
          if (currentMode != mode) break;

          R = 255;
          G = 255;
          B = 255;
          pixelSprinkle(2, 0, 2, 15);

          timeCheck();
        }
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 7)//CHORUS C2
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        while (elapsedMillis < (oneBar * 2))
        {
          randomTriangle(2, 0, true, 600);
          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          boxPin(2, 0, true, 400);
          timeCheck();
        }

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//C2 REPRISE
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        while (elapsedMillis < (oneBar * 2))
        {
          randomTriangle(2, 0, false, 1200);
          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          boxPin(2, 0, false, 450);
          timeCheck();
        }

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 5) && segment == 9)//MORPH
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        rotatingSquare(2, false, false, 0, false, 500);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 5) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 10)//CHORUS C3
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        while (elapsedMillis < (oneBar * 2))
        {
          randomTriangle(2, 0, true, 600);
          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          boxPin(2, 0, true, 400);
          timeCheck();
        }

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//C3 REPRISE
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        while (elapsedMillis < (oneBar * 2))
        {
          randomTriangle(2, 0, false, 1200);
          timeCheck();
        }
        while (elapsedMillis >= (oneBar * 2) && elapsedMillis < (oneBar * 4))
        {
          boxPin(2, 0, false, 450);
          timeCheck();
        }

        timeCheck();
      }

      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar) && segment == 12)//END
      {
        buttonCheck();
        if (currentMode != mode) break;

        //do this
        rotatingSquare(1, false, false, 0, false, 1250);

        timeCheck();
      }

      if (elapsedMillis >= (oneBar) && segment == 12) segment = 0, count = 1;

      //HARD END
      matrix.fillScreen(0);
      matrix.show();
      break;

    case 7:
      matrix.fillScreen(0);
      matrix.show();
      break;

    case 8:
      matrix.fillScreen(0);
      matrix.show();
      break;

    case 9://TEXT SCROLL - Jump into the Light
      if (segment == 1)
      {
        R = 0;
        G = 255;
        B = 255;
        titleScroll(2, 20);
      }
      /* if (segment == 2)
        {
         seoulScroll(35);
        }
        if (segment == 3)
        {
         webFestScroll(30);
        }*/
      if (segment == 4)
      {
        heartBridge();
      }
      buttonCheck;
      break;

    case 10://HEART FADE
      bpm = 120;
      oneBeat = (60000 / bpm);

      fadeUp(1, 3, (oneBeat * 3));

      buttonCheck();
      break;

    case 11://void circleDroplet(bool multi, unsigned long interval)
      bpm = 120;
      oneBeat = (60000 / bpm);

      circleDroplet(true, 1200);

      buttonCheck();
      break;


    case 12://SHOCK WAVE void sineLarson(bool multi, uint8_t frame, float frequency)
      bpm = 120;
      oneBeat = (60000 / bpm);

      /*R = 255;
        G = 255;
        B = 70;
        RA = 165;
        GA = 20;
        BA = 0;*/
      sineLarson(true, 5, 0.8);

      buttonCheck();
      break;

    case 13://void fuckTrumpScroll(bool multi, bool variant, unsigned long interval)
      bpm = 120;
      oneBeat = (60000 / bpm);
      timeCheck();
      currentMode = mode;

      fuckTrumpScroll(true, false, 25);
      buttonCheck();
      break;

    case 14://void randomRectangle(uint8_t multi, uint8_t frame, bool blackOut, bool fat, uint8_t scaler,  unsigned long interval)
      bpm = 120;
      oneBeat = (60000 / bpm);

      randomRectangle(2, 5, false, true, 5, 80);

      buttonCheck();
      break;

    case 15://heartbridge
      heartBridge();
      buttonCheck();
      break;

    case 16://bottom right black out
      matrix.fillScreen(0);
      matrix.show();
      buttonCheck();
      break;

    default://clear screen
      matrix.fillScreen(0);
      matrix.show();
      buttonCheck();
      break;

  }
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void eightCount(unsigned long interval)
{
  matrix.setTextSize(3);//1 is standard
  //matrix.setTextColor(colors[pass]);
  matrix.setTextColor(matrix.Color(R, G, B));
  matrix.fillScreen(0);
  matrix.setCursor(7, 4);
  matrix.print(count);
  matrix.show();
  timeCheck();

  currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    count++;
    if (count > 8) count = 1;
  }
}

void timeCheck()
{
  currentMillis = millis();//check the time
  elapsedMillis = currentMillis - reStartMillis;//check if enough time has passed
}

void zeroOut()
{
  x = matrix.width();
  y = matrix.height();
  i = 0;
  j = 0;
  part = 0;
  segment = 1;
  side = 2;
  pass = 0;
  count = 1;
  ready = true;
  currentMillis = millis();//check the time
  reStartMillis = currentMillis;
  previousMillis = currentMillis;
  elapsedMillis = 0;
  matrix.fillScreen(0);
}

void segmentZero(bool blackOut)
{
  reStartMillis = millis();
  previousMillis = millis();
  timeCheck();
  currentMode = mode;
  part = 0;
  i = 0;
  j = 0;
  x = matrix.width();
  y = matrix.height();
  if (blackOut == true) matrix.fillScreen(0);
}

void previousValues()
{
  previousMode = mode;
  previousCount = count;
  previousPass = pass;
  previousSegment = segment;
  previousX = x;
  previousY = y;
  previousRestartMillis = reStartMillis;
  timeCheck();
  previousElapsedMillis = elapsedMillis;
}

void restoreValues()
{
  x = previousX;
  y = previousY;
  mode = previousMode;
  count = previousCount;
  pass = previousPass;
  segment = previousSegment;
  elapsedMillis = previousElapsedMillis;
  reStartMillis = previousRestartMillis;
}

void buttonCheck()
{
  if (rf69.waitAvailableTimeout(2))//originally 100
  {
    // Should be a message for us now
    buf[RH_RF69_MAX_MESSAGE_LEN];
    len = sizeof(buf);

    if (! rf69.recv(buf, &len))
    {
      Serial.println("Receive failed");
      return;
    }

    rf69.printBuffer("Received: ", buf, len);
    buf[len] = 0;

    previousValues();
    mode = buf[0];
    zeroOut();
  }
}

void linearInterpol(int spaceMarker, int spaceRangeLow, int spaceRangeHigh)
{
  //gradient from one color to another using spacial relatoionships

  uint8_t red = lerp(spaceMarker, spaceRangeLow, spaceRangeHigh, 0, R);
  uint8_t green = lerp(spaceMarker, spaceRangeLow, spaceRangeHigh, 0, G);
  uint8_t blue = lerp(spaceMarker, spaceRangeLow, spaceRangeHigh, 0, B);
}

void getRandom()
{
  R = random(255);
  G = random(255);
  B = random(255);
  RA = random(255);
  GA = random(255);
  BA = random(255);
  X = random(matrix.width());
  Y = random(matrix.height());
}

void getCircleRandom()
{
  R = random(100, 255);
  G = random(100, 255);
  B = random(100, 255);
  RA = random(100);
  GA = random(100);
  BA = random(100);
  X = random(matrix.width());
  Y = random(matrix.height());
}

void getRectangleRandom()
{
  XA = random(matrix.width());//width
  YA = random(matrix.height());//height
  X = random(-6, matrix.width());
  Y = random(-6, matrix.height());
}

void getBlackRandom()
{
  blackXA = random(matrix.width());//width
  blackYA = random(matrix.height());//height
  blackX = random(-6, matrix.width());
  blackY = random(-6, matrix.height());
}

void seoulScroll(unsigned long interval)
{
  //add SEOUL logo
  matrix.drawBitmap(x, 0, seoulS, 86, 30, (matrix.Color(255, 0, 0)));
  matrix.drawBitmap(x, 0, seoulE, 86, 30, (matrix.Color(255, 150, 0)));
  matrix.drawBitmap(x, 0, seoulO, 86, 30, (matrix.Color(0, 40, 150)));
  matrix.drawBitmap(x, 0, seoulU, 86, 30, (matrix.Color(0, 150, 0)));
  matrix.drawBitmap(x, 0, seoulL, 86, 30, (matrix.Color(40, 0, 150)));
  matrix.drawBitmap(x, 0, seoulWhiteOverlay, 86, 30, (matrix.Color(200, 200, 200)));
  matrix.show();

  //add scrolling mechanics from sineLarson waveScroll
  currentMillis = millis();
  if (currentMillis - previousMillis >= (interval))//number adjusts speed
  {
    previousMillis = currentMillis;
    matrix.fillScreen(0);
    x--;
  }
  if (x <= -86)
  {
    x = matrix.width();
    segment = 3;//special bit added for logo scrolling
  }
}

void webFestScroll(unsigned long interval)
{
  currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    matrix.setTextSize(1);
    matrix.setTextWrap(false);

    matrix.setTextColor(matrix.Color(200, 200, 200));
    matrix.fillScreen(matrix.Color(0, 0, 0));
    matrix.setCursor(x, 18);
    matrix.print(F("Webfest"));
    matrix.show();

    if (--x < -42)//number of characters *6 *textSize
    {
      x = matrix.width();
      segment = 4;//special bit added for logo scrolling
    }
  }
}

void titleScroll(uint8_t multi, unsigned long interval)//changed for jump into the light
{
  currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    matrix.setTextSize(2);
    matrix.setTextWrap(false);

    if (multi == 1)
    {
      getRandom();
      matrix.setTextColor(matrix.Color(R, G, B));
      matrix.fillScreen(matrix.Color(0, 0, 0));
      matrix.setCursor(x, 8);
      matrix.print(F("JUMP INTO THE LIGHT"));
      matrix.show();
    }

    if (multi == 2)
    {
      matrix.setTextColor(Wheel(j));
      matrix.fillScreen(matrix.Color(0, 0, 0));
      matrix.setCursor(x, 8);
      matrix.print(F("JUMP INTO THE LIGHT"));
      matrix.show();
      j++;
      if (j > 255) j = 0;
    }

    if (--x < -342)//number of characters *6 *textSize
    {
      x = matrix.width();
      segment = 4;//special bit added for logo scrolling
    }
  }
}

void fuckTrumpScroll(bool multi, bool variant, unsigned long interval)
{
  currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    matrix.setTextSize(3);
    matrix.setTextWrap(false);
    if (multi == true)
    {
      j = j + 2;
      if (j > 255) j = 0;
      matrix.setTextColor(Wheel(j));
    }
    if (multi == false) matrix.setTextColor(matrix.Color(R, G, B));

    if (variant == false) matrix.fillScreen(matrix.Color(0, 0, 0));
    if (variant == true)
    {
      R = 200;
      G = 230;
      B = 50;
      RA = 0;
      GA = 50;
      BA = 255;
      sineLarson(false, 0, 0.7);
    }
    //matrix.setTextColor(matrix.Color(255, 0, 0));
    //matrix.fillScreen(matrix.Color(0, 0, 0));
    matrix.setCursor(x, 5);
    matrix.print(F("FUCK TRUMP"));
    matrix.show();

    if (--x < -180) x = matrix.width();//number of characters *6 *textSize
  }
}

void allAroundScroll(bool multi, uint8_t version, unsigned long interval)
{
  matrix.fillScreen(matrix.Color(0, 0, 0));
  currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    matrix.setTextWrap(false);
    if (multi == true) getRandom();

    matrix.setTextColor(matrix.Color(255, 0, 0));
    if (part == 0)
    {
      matrix.setTextSize(1);
      matrix.setCursor(7, 0);
      matrix.print("ALL");
      matrix.setCursor(x, 7);
      matrix.setTextSize(2);
      matrix.print(F("around"));
      matrix.setCursor(6, 23);
      matrix.setTextSize(1);
      matrix.print("YOU");
      matrix.show();

      if (--x < -72) x = matrix.width(), part = 1; //number of characters *6 *textSize
    }

    if (part == 1)
    {
      if (multi == true) getRandom();
      matrix.setTextColor(matrix.Color(255, 0, 255));

      matrix.setTextSize(1);
      matrix.setCursor(7, 0);
      matrix.print("ALL");
      matrix.setCursor(x, 7);
      matrix.setTextSize(2);
      matrix.print(F("around"));
      matrix.setCursor(9, 23);
      matrix.setTextSize(1);
      matrix.print("ME");
      matrix.show();

      if (--x < -72) x = matrix.width(), part = 2; //number of characters *6 *textSize
    }

    if (part == 2 && version == 0)
    {
      if (multi == true) getRandom();
      matrix.setTextColor(matrix.Color(255, 255, 0));
      matrix.setTextSize(1);
      matrix.setCursor(7, 0);
      matrix.print("ALL");
      matrix.setCursor(x, 7);
      matrix.setTextSize(2);
      matrix.print(F("around"));
      matrix.setCursor(x, 23);
      matrix.setTextSize(1);
      matrix.print(F("EVERYTHING YOU NEED"));
      matrix.show();

      if (--x < -114) x = matrix.width(), part = 0; //number of characters *6 *textSize
    }
    if (part == 2 && version == 1)
    {
      if (multi == true) getRandom();
      matrix.setTextColor(matrix.Color(255, 255, 0));
      matrix.setTextSize(1);
      matrix.setCursor(7, 0);
      matrix.print("ALL");
      matrix.setCursor(x, 7);
      matrix.setTextSize(2);
      matrix.print(F("around"));
      matrix.setCursor(x, 23);
      matrix.setTextSize(1);
      matrix.print(F("EVERYTHING WE SEE"));
      matrix.show();

      if (--x < -114) x = matrix.width(), part = 0; //number of characters *6 *textSize
    }
    if (part == 2 && version == 2)
    {
      if (multi == true) getRandom();
      matrix.setTextColor(matrix.Color(255, 255, 0));
      matrix.setTextSize(1);
      matrix.setCursor(7, 0);
      matrix.print("ALL");
      matrix.setCursor(x, 7);
      matrix.setTextSize(2);
      matrix.print(F("around"));
      matrix.setCursor(x, 23);
      matrix.setTextSize(1);
      matrix.print(F("EVERYTHING WE NEED"));
      matrix.show();

      if (--x < -114) x = matrix.width(); //number of characters *6 *textSize
    }
  }
}

void beautifulScroll(uint8_t multi, uint8_t version, uint8_t scaler, unsigned long interval)
{
  float frequency = 0.9;
  uint8_t bandWidth = 25;
  if (version != 1) matrix.fillScreen(matrix.Color(0, 0, 0));
  currentMillis = millis();

  matrix.setTextWrap(false);

  if (multi == 0) matrix.setTextColor(matrix.Color(0, 0, 0));//blackOut
  if (multi == 1) matrix.setTextColor(matrix.Color(R, G, B));//random
  if (multi == 2) matrix.setTextColor(Wheel(j));//rainbow

  if (version == 1)
  {
    //sineDroplet(2, true, 0.9, 25, 0, 10);
    X = matrix.width() / 2; //centers circle
    Y = matrix.height() / 2;
    //sine wave sets the color
    float currentS = millis() / 1000.0;
    float phase = (float)i / (float)(matrix.width() - 1) * 2.0 * PI;//use this number to change the band size
    float t = sin(2.0 * PI * frequency * currentS + phase);
    R = 0;
    G = 0;
    B = 255;
    RA = 128;
    GA = 128;
    BA = 0;
    red = lerp(t, -1.0, 1.0, R, RA);
    green = lerp(t, -1.0, 1.0, G, GA);
    blue = lerp(t, -1.0, 1.0, B, BA);

    matrix.drawCircle(X, Y, i, (matrix.Color(red, green, blue))); //draw the circles
    i++;
    if (i > bandWidth)
    {
      i = 1;
      if (multi == 1) getCircleRandom();
    }
  }
  matrix.setTextSize(2);
  matrix.setCursor(x, 8);
  matrix.print("BEAUTIFUL");
  matrix.show();

  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    if (--x < -108) x = matrix.width(); //number of characters *6 *textSize
    if (multi == 2)  if ((j = j + scaler) > 255) j = 0;
  }
}

void everythingScroll(uint8_t multi, uint8_t scaler, unsigned long interval)
{
  currentMillis = millis();
  matrix.fillScreen(matrix.Color(0, 0, 0));

  if (multi == 0) matrix.setTextColor(matrix.Color(0, 0, 0));//blackOut
  if (multi == 1) matrix.setTextColor(matrix.Color(R, G, B));//random
  if (multi == 2) matrix.setTextColor(Wheel(j));//rainbow

  matrix.setTextWrap(false);
  matrix.setTextSize(1);
  matrix.setCursor(0, 1);
  matrix.print("CAUSE");
  matrix.setCursor(x - 15, 11);
  matrix.print(F("EVERYTHING"));
  matrix.setCursor(9, 22);
  matrix.setTextSize(1);
  matrix.print("IS");
  matrix.show();

  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    if (--x < -60) x = matrix.width(); //number of characters *6 *textSize
    if (multi == 2)  if ((j = j + scaler) > 255) j = 0;
  }
}

void radioScroll(uint8_t multi, uint8_t scaler, unsigned long interval)
{
  matrix.fillScreen(matrix.Color(0, 0, 0));
  currentMillis = millis();

  matrix.setTextWrap(false);

  if (multi == 0) matrix.setTextColor(matrix.Color(0, 0, 0));//blackOut
  if (multi == 1) matrix.setTextColor(matrix.Color(R, G, B));//random
  if (multi == 2) matrix.setTextColor(Wheel(j));//rainbow

  matrix.setTextSize(1);

  matrix.setCursor(0, y);
  matrix.print("RADIO");
  matrix.setCursor(0, y + 7);
  matrix.print("GA GA");
  matrix.setCursor(0, y + 21);
  matrix.print("RADIO");
  matrix.setCursor(6, y + 28);
  matrix.print("GOO");
  matrix.setCursor(6, y + 35);
  matrix.print("GOO");
  matrix.setCursor(0, y + 49);
  matrix.print("RADIO");
  matrix.setCursor(0, y + 56);
  matrix.print("GA GA");
  matrix.setCursor(0, y + 96);
  matrix.print("RADIO");
  matrix.setCursor(0, y + 103);
  matrix.print("GA GA");
  matrix.setCursor(0, y + 117);
  matrix.print("RADIO");
  matrix.setCursor(3, y + 124);
  matrix.print("BLAH");
  matrix.setCursor(3, y + 131);
  matrix.print("BLAH");


  //matrix.print("All we hear is RADIO GA GA   RADIO GOO GOO   RADIO GA GA    All we hear is RADIO BLAH BLAH"));
  matrix.show();

  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    if (--y < -131) y = matrix.height(); //number of characters *6 *textSize
    if (multi == 2)  if ((j = j + scaler) > 255) j = 0;
  }
}

void radioSecondScroll(uint8_t multi, uint8_t scaler, unsigned long interval)
{
  matrix.fillScreen(matrix.Color(0, 0, 0));
  currentMillis = millis();

  matrix.setTextWrap(false);

  if (multi == 0) matrix.setTextColor(matrix.Color(0, 0, 0));//blackOut
  if (multi == 1) matrix.setTextColor(matrix.Color(R, G, B));//random
  if (multi == 2) matrix.setTextColor(Wheel(j));//rainbow

  matrix.setTextSize(1);
  matrix.setCursor(0, y);
  matrix.print("RADIO");
  matrix.setCursor(0, y + 7);
  matrix.print("GA GA");
  matrix.setCursor(0, y + 21);
  matrix.print("RADIO");
  matrix.setCursor(6, y + 28);
  matrix.print("GOO");
  matrix.setCursor(6, y + 35);
  matrix.print("GOO");
  matrix.setCursor(0, y + 49);
  matrix.print("RADIO");
  matrix.setCursor(0, y + 56);
  matrix.print("GA GA");

  matrix.setCursor(0, y + 100);
  matrix.print("RADIO");
  matrix.setCursor(0, y + 107);
  matrix.print("GA GA");
  matrix.setCursor(0, y + 121);
  matrix.print("RADIO");
  matrix.setCursor(6, y + 128);
  matrix.print("GOO");
  matrix.setCursor(6, y + 135);
  matrix.print("GOO");
  matrix.setCursor(0, y + 149);
  matrix.print("RADIO");
  matrix.setCursor(0, y + 156);
  matrix.print("GA GA");
  matrix.setCursor(0, y + 186);
  matrix.print("RADIO");
  matrix.setCursor(0, y + 193);
  matrix.print("GA GA");
  matrix.setCursor(0, y + 207);
  matrix.print("RADIO");
  matrix.setCursor(3, y + 214);
  matrix.print("BLAH");
  matrix.setCursor(3, y + 221);
  matrix.print("BLAH");


  //matrix.print("All we hear is RADIO GA GA   RADIO GOO GOO   RADIO GA GA    All we hear is RADIO BLAH BLAH"));
  matrix.show();

  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    if (--y < -221) y = matrix.height(); //number of characters *6 *textSize
    if (multi == 2)  if ((j = j + scaler) > 255) j = 0;
  }
}

void singScroll(uint8_t multi, uint8_t scaler, unsigned long interval)
{
  currentMillis = millis();
  matrix.fillScreen(matrix.Color(0, 0, 0));

  if (multi == 0 || multi == 1) matrix.setTextColor(matrix.Color(R, G, B));
  if (multi == 2) matrix.setTextColor(Wheel(j));//rainbow

  matrix.setTextWrap(false);
  matrix.setTextSize(1);
  matrix.setCursor(3, 1);
  matrix.print("SING");

  if (currentMillis - previousMillis >= (interval / 4))
  {
    matrix.setCursor(3, 11);
    matrix.print("SING");
  }
  if (currentMillis - previousMillis >= (interval / 4) * 2)
  {
    matrix.setCursor(3, 21);
    matrix.print("SING");
  }

  matrix.show();

  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    if (multi == 2)  if ((j = j + scaler) > 255) j = 0;
  }
}

void heartBridge()
{
  currentMode = mode;
  //white sprinkle with red heart
  matrix.fillScreen( matrix.Color(50, 50, 50));
  for (uint8_t pass = 0; pass < 5; pass++)
  {
    buttonCheck();
    if (currentMode != mode) break;

    for (uint8_t count = 0; count < 100; count++)
    {
      buttonCheck();
      if (currentMode != mode) break;

      getRandom();
      matrix.drawPixel(X, Y, matrix.Color(R, R, R));
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(255, 0, 0)));
      matrix.show();
    }
  }

  //red sprinkle with white heart
  matrix.fillScreen( matrix.Color(50, 0, 0));
  for (uint8_t pass = 0; pass < 5; pass++)
  {
    buttonCheck();
    if (currentMode != mode)
    {
      break;
    }
    for (uint8_t count = 0; count < 100; count++)
    {
      buttonCheck();
      if (currentMode != mode)
      {
        break;
      }
      getRandom();
      matrix.drawPixel(X, Y, matrix.Color(R, 0, 0));
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(255, 255, 255)));
      matrix.show();
      if (count == 99)
      {
        segment = 1;//special bit added for logo scrolling
      }
    }
  }
}

void boxPin(uint8_t multi, uint8_t frame, bool blackOut, unsigned long interval)
{
  if (part == 0)
  {
    //do this
    if (multi == 0 || multi == 1)
    {
      matrix.drawLine(0, i, (matrix.width() - 1), 0, matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.drawLine(0, i, (matrix.width() - 1), 0, Wheel(j));
    }
    if (frame == 1)
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    if (frame == 2)
    {
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();

    //then advance the counters if the interval has passed
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / matrix.height() - 1))
    {
      previousMillis = currentMillis;
      if (blackOut == true) matrix.fillScreen(matrix.Color(0, 0, 0));
      i++;
      if (i >= matrix.width() - 1)
      {
        i = 0;
        part = 1;
        if (multi == 1) getRandom();
      }
      if (multi == 2)
      {
        j = j + 2;
        if (j > 255) j = 0;
      }
    }
  }
  if (part == 1)
  {
    //do this
    if (multi == 0 || multi == 1)
    {
      matrix.drawLine((matrix.width() - 1), 0, i, (matrix.height() - 1), matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.drawLine((matrix.width() - 1), 0, i, (matrix.height() - 1), Wheel(j));
    }
    if (frame == 1)
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    if (frame == 2)
    {
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();

    //then advance the counters if the interval has passed
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / matrix.width() - 1))
    {
      previousMillis = currentMillis;
      if (blackOut == true) matrix.fillScreen(matrix.Color(0, 0, 0));
      i++;
      if (i >= matrix.width() - 1)
      {
        i = matrix.width() - 1;
        part = 2;
        if (multi == 1) getRandom();
      }
      if (multi == 2)
      {
        j = j + 2;
        if (j > 255) j = 0;
      }
    }
  }
  if (part == 2)
  {
    //do this
    if (multi == 0 || multi == 1)
    {
      matrix.drawLine(i, 0, (x - 1), (y - 1), matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.drawLine(i, 0, (x - 1), (y - 1), Wheel(j));
    }
    if (frame == 1)
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    if (frame == 2)
    {
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();
    //then advance the counters if the interval has passed
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / matrix.width() - 1))
    {
      previousMillis = currentMillis;
      if (blackOut == true) matrix.fillScreen(matrix.Color(0, 0, 0));
      i--;
      if (i < 0)
      {
        i = 0;
        part = 3;
        if (multi == 1) getRandom();
      }
      if (multi == 2)
      {
        j = j + 2;
        if (j > 255) j = 0;
      }
    }
  }
  if (part == 3)
  {
    //do this
    if (multi == 0 || multi == 1)
    {
      matrix.drawLine(0, i, (matrix.width() - 1), (matrix.height() - 1), matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.drawLine(0, i, (matrix.width() - 1), (matrix.height() - 1), Wheel(j));
    }
    if (frame == 1)
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    if (frame == 2)
    {
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();
    //then advance the counters if the interval has passed
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / matrix.height() - 1))
    {
      previousMillis = currentMillis;
      if (blackOut == true) matrix.fillScreen(matrix.Color(0, 0, 0));
      i++;
      if (i > matrix.height() - 1)
      {
        i = matrix.height() - 1;
        part = 4;
        if (multi == 1) getRandom();
      }
      if (multi == 2)
      {
        j = j + 2;
        if (j > 255) j = 0;
      }
    }
  }
  if (part == 4)
  {
    //do this
    if (multi == 0 || multi == 1)
    {
      matrix.drawLine(0, (matrix.height() - 1), (matrix.width() - 1), i, matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.drawLine(0, (matrix.height() - 1), (matrix.width() - 1), i, Wheel(j));
    }
    if (frame == 1)
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    if (frame == 2)
    {
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();
    //then advance the counters if the interval has passed
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / matrix.height() - 1))
    {
      previousMillis = currentMillis;
      if (blackOut == true) matrix.fillScreen(matrix.Color(0, 0, 0));
      i--;
      if (i  < 0)
      {
        i = matrix.width() - 1;
        part = 5;
        if (multi == 1) getRandom();
      }
      if (multi == 2)
      {
        j = j + 2;
        if (j > 255) j = 0;
      }
    }
  }
  if (part == 5)
  {
    //do this
    if (multi == 0 || multi == 1)
    {
      matrix.drawLine(0, (matrix.height() - 1), i, 0, matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.drawLine(0, (matrix.height() - 1), i, 0, Wheel(j));
    }
    if (frame == 1)
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    if (frame == 2)
    {
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();
    //then advance the counters if the interval has passed
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / matrix.width() - 1))
    {
      previousMillis = currentMillis;
      if (blackOut == true) matrix.fillScreen(matrix.Color(0, 0, 0));
      i--;
      if (i  < 0)
      {
        i = 0;
        part = 6;
        if (multi == 1) getRandom();
      }
      if (multi == 2)
      {
        j = j + 2;
        if (j > 255) j = 0;
      }
    }
  }
  if (part == 6)
  {
    //do this
    if (multi == 0 || multi == 1)
    {
      matrix.drawLine(0, 0, i, (matrix.height() - 1), matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.drawLine(0, 0, i, (matrix.height() - 1), Wheel(j));
    }
    if (frame == 1)
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    if (frame == 2)
    {
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();
    //then advance the counters if the interval has passed
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / matrix.width() - 1))
    {
      previousMillis = currentMillis;
      if (blackOut == true) matrix.fillScreen(matrix.Color(0, 0, 0));
      i++;
      if (i  > matrix.width() - 1)
      {
        i = matrix.height() - 1;
        part = 7;
        if (multi == 1) getRandom();
      }
      if (multi == 2)
      {
        j = j + 2;
        if (j > 255) j = 0;
      }
    }
  }
  if (part == 7)
  {
    //do this
    if (multi == 0 || multi == 1)
    {
      matrix.drawLine(0, 0, (matrix.width() - 1), i, matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.drawLine(0, 0, (matrix.width() - 1), i, Wheel(j));
    }
    if (frame == 1)
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    if (frame == 2)
    {
      matrix.drawBitmap(0, 0, heartOne, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();
    //then advance the counters if the interval has passed
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / matrix.height() - 1))
    {
      previousMillis = currentMillis;
      if (blackOut == true) matrix.fillScreen(matrix.Color(0, 0, 0));
      i--;
      if (i  < 0)
      {
        i = 0;
        part = 0;
        if (multi == 1) getRandom();
        if (multi == 2)
        {
          j = j + 2;
          if (j > 255) j = 0;
        }
      }
    }
  }
}

void strobe(uint8_t multi, uint8_t frame, unsigned long interval)
{
  if (part == 0)
  {
    if (multi == 0 || multi == 1)
    {
      matrix.fillScreen(matrix.Color(R, G, B));
    }
    if (multi == 2)
    {
      matrix.fillScreen(Wheel(j));
    }
    if (frame == 1) matrix.drawBitmap(0, 0, radio, 30, 30, (matrix.Color(0, 0, 0)));
    matrix.show();

    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / 2))
    {
      previousMillis = currentMillis;
      part = 1;
    }
  }

  if (part == 1)
  {
    matrix.fillScreen(matrix.Color(0, 0, 0));
    matrix.show();

    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / 2))
    {
      previousMillis = currentMillis;
      part = 0;
      if (multi == 1) getRandom();
      if (multi == 2)
      {
        j++;
        if (j > 255) j = 0;
      }
    }
  }
}


void circleDroplet(bool multi, unsigned long interval)
{
  //do this
  matrix.fillScreen(matrix.Color(0, 0, 0));
  matrix.fillCircle(X, Y, i, (matrix.Color(RA, GA, BA)));
  matrix.drawCircle(X, Y, i, (matrix.Color(R, G, B)));
  matrix.drawCircle(X, Y, i - 2, (matrix.Color(RA, GA, BA)));
  matrix.drawCircle(X, Y, i - 4, (matrix.Color(R, G, B)));
  matrix.drawCircle(X, Y, i - 6, (matrix.Color(RA, GA, BA)));
  matrix.drawCircle(X, Y, i - 8, (matrix.Color(R, G, B)));
  matrix.drawCircle(X, Y, i - 10, (matrix.Color(RA, GA, BA)));
  matrix.drawCircle(X, Y, i - 12, (matrix.Color(R, G, B)));
  matrix.fillCircle(X, Y, i - 14, (matrix.Color(0, 0, 0)));
  matrix.show();

  //then check the time and advance counters if ready
  currentMillis = millis();
  if (currentMillis - previousMillis > (interval / 40))
  {
    previousMillis = currentMillis;
    i++;
    if (i > 40)//34 is a semi randomly chosen number based on generally clearing my screen
    {
      i = 0;
      if (multi == true) getCircleRandom();
    }
  }
}

void randomTriangle(uint8_t multi, uint8_t frame, bool blackOut, unsigned long interval)
{
  if (part == 0)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / 5))
    {
      previousMillis = currentMillis;
      part = 1;
      XA = random(x);
      YA = random(y);
      XB = random(x);
      YB = random(y);
      XC = random(x);
      YC = random(y);

      if (blackOut == true)
      {
        matrix.fillScreen(matrix.Color(0, 0, 0));
      }

      if (multi == 0)
      {
        matrix.fillTriangle((XA - 2), (YA - 2), (XB - 2), (YB - 2), (XC - 2), (YC - 2), (matrix.Color(R, G, B)));
      }
      if (multi == 1)
      {
        getRandom();
        matrix.fillTriangle((XA - 2), (YA - 2), (XB - 2), (YB - 2), (XC - 2), (YC - 2), (matrix.Color(R, G, B)));
      }
      if (multi == 2)
      {
        matrix.fillTriangle((XA - 2), (YA - 2), (XB - 2), (YB - 2), (XC - 2), (YC - 2), (Wheel(j)));
        j = j + 8;
        if (j > 255) j = 0;
      }

      if (frame == 1)
      {
        matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
      }

      matrix.show();
    }
  }

  if (part == 1)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / 5))
    {
      previousMillis = currentMillis;
      part = 2;
      if (multi == 0)
      {
        matrix.fillTriangle((XA + 2), (YA + 2), (XB + 2), (YB + 2), (XC + 2), (YC + 2), (matrix.Color(R, G, B)));
      }
      if (multi == 1)
      {
        getRandom();
        matrix.fillTriangle((XA + 2), (YA + 2), (XB + 2), (YB + 2), (XC + 2), (YC + 2), (matrix.Color(R, G, B)));
      }
      if (multi == 2)
      {
        matrix.fillTriangle((XA + 2), (YA + 2), (XB + 2), (YB + 2), (XC + 2), (YC + 2), (Wheel(j)));
        j = j + 8;
        if (j > 255) j = 0;
      }

      if (frame == 1)
      {
        matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
      }

      matrix.show();
    }
  }

  if (part == 2)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / 5))
    {
      previousMillis = currentMillis;
      part = 3;
      if (multi == 0)
      {
        matrix.fillTriangle((XA + 2), (YA - 2), (XB + 2), (YB - 2), (XC + 2), (YC - 2), (matrix.Color(R, G, B)));
      }
      if (multi == 1)
      {
        getRandom();
        matrix.fillTriangle((XA + 2), (YA - 2), (XB + 2), (YB - 2), (XC + 2), (YC - 2), (matrix.Color(R, G, B)));
      }
      if (multi == 2)
      {
        matrix.fillTriangle((XA + 2), (YA - 2), (XB + 2), (YB - 2), (XC + 2), (YC - 2), (Wheel(j)));
        j = j + 8;
        if (j > 255) j = 0;
      }

      if (frame == 1)
      {
        matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
      }

      matrix.show();
      //delay(wait);
    }
  }

  if (part == 3)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / 5))
    {
      previousMillis = currentMillis;
      part = 4;

      if (multi == 0)
      {
        matrix.fillTriangle((XA - 2), (YA + 2), (XB - 2), (YB + 2), (XC - 2), (YC + 2), (matrix.Color(R, G, B)));
      }
      if (multi == 1)
      {
        getRandom();
        matrix.fillTriangle((XA - 2), (YA + 2), (XB - 2), (YB + 2), (XC - 2), (YC + 2), (matrix.Color(R, G, B)));
      }
      if (multi == 2)
      {
        matrix.fillTriangle((XA - 2), (YA + 2), (XB - 2), (YB + 2), (XC - 2), (YC + 2), (Wheel(j)));
        j = j + 8;
        if (j > 255) j = 0;
      }

      if (frame == 1)
      {
        matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
      }

      matrix.show();
    }
  }

  if (part == 4)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis > (interval / 5))
    {
      previousMillis = currentMillis;
      part = 0;
      if (multi == 0)
      {
        matrix.fillTriangle(XA, YA, XB, YB, XC, YC, (matrix.Color(R, G, B)));
      }
      if (multi == 1)
      {
        getRandom();
        matrix.fillTriangle(XA, YA, XB, YB, XC, YC, (matrix.Color(R, G, B)));
      }
      if (multi == 2)
      {
        matrix.fillTriangle(XA, YA, XB, YB, XC, YC, (Wheel(j)));
        j = j + 8;
        if (j > 255) j = 0;
      }

      if (frame == true)
      {
        matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
      }

      matrix.show();
    }
  }
}

void boxMarch(uint8_t multi, uint8_t frame, bool blackOut, unsigned long interval)
{
  currentMillis = millis();
  //do your stuff
  if (multi == 0 || multi == 1)
  {
    matrix.drawRect(((matrix.width() / 2) - pass) - 1, ((matrix.height() / 2) - pass) - 1, side, side, matrix.Color(R, G, B));
  }
  if (multi == 2)
  {
    matrix.drawRect(((matrix.width() / 2) - pass) - 1, ((matrix.height() / 2) - pass) - 1, side, side, Wheel(j));
  }

  if (frame == 1)
  {
    matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
  }
  if (frame == 2)
  {
    matrix.drawBitmap(0, 0, radio, 30, 30, (matrix.Color(0, 0, 0)));
  }

  matrix.show();

  //advance variables
  if (currentMillis - previousMillis > (interval / (matrix.width() / 2) - 1))
  {
    previousMillis = currentMillis;

    if (multi == 1) getRandom();
    if (multi == 2)
    {
      j = j + 8;
      if (j > 255) j = 0;
    }

    side = side + 2;
    if (side > matrix.width()) side = 2;
    pass++;
    if (pass > (matrix.width() / 2) - 1)
    {
      pass = 0;
      if (blackOut == true) matrix.fillScreen(0);
    }

    timeCheck();
  }
}


void sineLarson(bool multi, uint8_t frame, float frequency)//larson with a sine wave
{
  uint8_t n = matrix.width();//number of rows
  float currentS = millis() / 1000.0;

  for (int i = 0; i < n; i++)
  {
    float phase = (float)i / (float)(n - 1) * 2.0 * PI;
    float t = sin(2.0 * PI * frequency * currentS + phase);

    red = lerp(t, -1.0, 1.0, R, RA);
    green = lerp(t, -1.0, 1.0, G, GA);
    blue = lerp(t, -1.0, 1.0, B, BA);

    matrix.drawLine(0, i, matrix.width(), i, matrix.Color(red, green, blue));
  }
  if (frame == 1)
  {
    matrix.drawBitmap(0, y, cloudsOne, 30, 41, (matrix.Color(255, 255, 255)));
  }
  if (frame == 2)
  {
    matrix.setTextColor(matrix.Color(0, 0, 0));
    matrix.setTextSize(1);
    matrix.setCursor(3, 12);
    matrix.print("NEED");
  }
  if (frame == 3)
  {
    matrix.drawBitmap(0, y - 30, sunRise, 30, 90, (matrix.Color(0, 0, 0)));
  }
  if (frame == 4)
  {
    matrix.drawBitmap(x - 30, 0, heartRowFrame, 96, 30, (matrix.Color(0, 0, 0)));
  }
  if (frame == 5)
  {
    matrix.drawBitmap(x - matrix.width(), 0, waveScroll, 150, 30, (matrix.Color(0, 0, 0)));
  }
  if (frame == 6)
  {
    matrix.drawBitmap(x - matrix.width(), 0, waveScroll, 150, 30, (matrix.Color(0, 0, 40)));
  }
  if (frame == 7)
  {
    matrix.drawBitmap(x - matrix.width(), 0, waveScroll, 150, 30, (matrix.Color(20, 0, 0)));
  }
  matrix.show();

  //check the time and advance counters
  currentMillis = millis();
  if (frame == 0)
  {
    if (currentMillis - previousMillis >= (3000))//number adjusts speed
    {
      previousMillis = currentMillis;
      if (multi == true) getRandom;

    }
  }
  if (frame == 3)
  {
    if (currentMillis - previousMillis >= (62))//number adjusts speed
    {
      previousMillis = currentMillis;
      y--;
    }
    if (y <= -30)
    {
      y = matrix.height();
      if (multi == true) getRandom();
    }
  }
  if (frame == 4)
  {
    if (currentMillis - previousMillis >= (28))//number adjusts speed
    {
      previousMillis = currentMillis;
      x--;
    }
    if (x <= -34)
    {
      x = matrix.width();
      if (multi == true) getRandom();
    }
  }
  if (frame == 5)
  {
    if (currentMillis - previousMillis >= (20))//number adjusts speed
    {
      previousMillis = currentMillis;
      x--;
    }
    if (x <= -90)
    {
      x = matrix.width();
      if (multi == true) getRandom();
    }
  }
  if (frame == 6)
  {
    if (currentMillis - previousMillis >= (25))//number adjusts speed
    {
      previousMillis = currentMillis;
      x--;
    }
    if (x <= -90)
    {
      x = matrix.width();
      if (multi == true) getRandom();
    }
  }
  if (frame == 7)
  {
    if (currentMillis - previousMillis >= (10))//number adjusts speed
    {
      previousMillis = currentMillis;
      x--;
    }
    if (x <= -90)
    {
      x = matrix.width();
      if (multi == true) getRandom();
    }
  }
}

void heartbeat(uint8_t multi, uint8_t frame, uint32_t interval) //abstracted from Circuit Playground 32u4 'Power' Reactor Sketch by Tony DiCola
{
  currentMillis = millis();

  uint32_t to = (currentMillis % interval);
  float xo = pow(M_E, -(float)to / (float)(interval / 4));

  red = lerp(xo, 1.0, 0.0, R, RA);
  green = lerp(xo, 1.0, 0.0, G, GA);
  blue = lerp(xo, 1.0, 0.0, B, BA);
  matrix.fillScreen(matrix.Color(red, green, blue));

  if (frame == 1) matrix.drawBitmap(0, - 30, sunRise, 30, 90, (matrix.Color(0, 0, 0)));
  if (frame == 2) matrix.drawBitmap(0, 0, heart, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 3) matrix.drawBitmap(0, 0, heartOneFrame, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 4) matrix.drawBitmap(0, 0, heart, 30, 30, (matrix.Color(0, 0, 0)));

  matrix.show();

  //advance variables
  timeCheck();
  if (currentMillis - previousMillis > (interval))
  {
    previousMillis = currentMillis;
    //if (multi == 1) getCircleRandom();
  }
}

void rotatingSquare(uint8_t multi, bool frame, bool sine, uint8_t variant, bool blackOut, unsigned long interval)
{
  //update the corner position
  uint8_t xa = i;
  uint8_t xb = (matrix.width() - 1);
  uint8_t xc = ((matrix.width() - 1) - i);
  uint8_t xd = 0;
  uint8_t ya = 0;
  uint8_t yb = i;
  uint8_t yc = (matrix.height() - 1);
  uint8_t yd = ((matrix.height() - 1) - i);

  //draw a background
  if (variant == 1)
  {
    matrix.fillScreen(matrix.Color(0, 50, 0));
    if (frame == 1)//add an overlay
    {
      matrix.drawBitmap(0, 0, radio, 30, 30, (matrix.Color(0, 0, 0)));
    }
  }
  if (variant == 2)
  {
    matrix.fillScreen(Wheel(j));
    if (frame == 1)//add an overlay
    {
      matrix.drawBitmap(0, 0, radio, 30, 30, (matrix.Color(0, 0, 0)));
    }
  }

  if (blackOut == true)//wipe the screen clean
  {
    matrix.fillScreen(0);
  }

  if (sine == true)//sine wave
  {
    float currentS = millis() / 1000.0;

    float frequency = 0.8;//try to set this elsewhere

    float phase = (float)i / (float)(x - 1) * 2.0 * PI;//use this number to change the band size
    float t = sin(2.0 * PI * frequency * currentS + phase);

    red = lerp(t, -1.0, 1.0, R, RA);
    green = lerp(t, -1.0, 1.0, G, GA);
    blue = lerp(t, -1.0, 1.0, B, BA);

    matrix.drawLine(xa, ya, xb, yb, (matrix.Color(red, green, blue)));
    matrix.drawLine(xb, yb, xc, yc, (matrix.Color(red, green, blue)));
    matrix.drawLine(xc, yc, xd, yd, (matrix.Color(red, green, blue)));
    matrix.drawLine(xd, yd, xa, ya, (matrix.Color(red, green, blue)));
  }

  if (multi == 0 || multi == 1)
  {
    matrix.drawLine(xa, ya, xb, yb, (matrix.Color(R, G, B)));
    matrix.drawLine(xb, yb, xc, yc, (matrix.Color(R, G, B)));
    matrix.drawLine(xc, yc, xd, yd, (matrix.Color(R, G, B)));
    matrix.drawLine(xd, yd, xa, ya, (matrix.Color(R, G, B)));
  }

  if (multi == 2)
  {
    matrix.drawLine(xa, ya, xb, yb, (Wheel(j)));
    matrix.drawLine(xb, yb, xc, yc, (Wheel(j)));
    matrix.drawLine(xc, yc, xd, yd, (Wheel(j)));
    matrix.drawLine(xd, yd, xa, ya, (Wheel(j)));
  }

  matrix.show();

  //check the time and update variables
  currentMillis = millis();
  if (currentMillis - previousMillis >= (interval / matrix.width()))
  {
    previousMillis = currentMillis;

    if (multi == 2 || variant == 2)
    {
      j++;
      if (j > 255) j = 0;
    }
    i++;
    if (i >= matrix.width())
    {
      i = 0;
      if (multi == 1) getRandom();
    }
  }
}

void sineDroplet(uint8_t multi, bool stationary, float frequency, uint8_t bandWidth, uint8_t frame, unsigned long interval)
{
  currentMillis = millis();
  while (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    buttonCheck();
    if (currentMode != mode)
    {
      break;
    }

    //sine wave sets the color
    float currentS = millis() / 1000.0;
    float phase = (float)i / (float)(x - 1) * 2.0 * PI;//use this number to change the band size
    float t = sin(2.0 * PI * frequency * currentS + phase);
    red = lerp(t, -1.0, 1.0, R, RA);
    green = lerp(t, -1.0, 1.0, G, GA);
    blue = lerp(t, -1.0, 1.0, B, BA);

    if (stationary == true)
    {
      X = matrix.width() / 2; //centers circle
      Y = matrix.height() / 2;
    }
    if (multi != 2) matrix.drawCircle(X, Y, i, (matrix.Color(red, green, blue))); //draw the circles
    if (multi == 2) matrix.drawCircle(X, Y, i, (Wheel(j)));

    if (frame == 1)//add an overlay
    {
      //matrix.drawBitmap(0, 0, heartOneInverse, 30, 30, (matrix.Color(0, 0, 0)));
      matrix.drawBitmap(0, -30, sunRise, 30, 90, (matrix.Color(0, 0, 0)));//insert frame data here
      matrix.drawBitmap(0, y, cloudsOne, 30, 40, (matrix.Color(255, 255, 255)));
    }
    if (frame == 2)//add an overlay
    {
      matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    }
    matrix.show();
    i++;
    if (frame != 0) y--;
    if (frame == 1 && y < -40) y = matrix.height();
    j = j + 8;
    if (j > 255) j = 0;
    if (i > bandWidth)
    {
      i = 1;
      if (multi == 1) getCircleRandom();
      if (multi != 1) X = random(matrix.width()), Y = random(matrix.height());
    }
  }
}

void pixelSprinkle(uint8_t multi, uint8_t frame, uint8_t blackOut, unsigned long interval)
{
  //do this
  if (multi == 0 || multi == 1)
  {
    matrix.drawPixel(X, Y, matrix.Color(R, G, B));
  }
  if (multi == 2)
    matrix.drawPixel(X, Y, Wheel(j));

  if (frame == 1)
  {
    matrix.drawBitmap(0, 0, sunOne, 30, 30, (matrix.Color(127, 127, 0)));
  }
  if (frame == 2)
  {
    matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
    //matrix.drawBitmap(0, 0, heartTwo, 30, 30, (matrix.Color(255, 255, 255)));
  }

  matrix.show();

  //check the time and advance the variables
  currentMillis = millis();
  if (currentMillis - previousMillis >= (interval))
  {
    previousMillis = currentMillis;
    for (i = 0; i < blackOut; i++)
    {
      XA = random(matrix.width());
      YA = random(matrix.height());
      matrix.drawPixel(XA, YA, matrix.Color(0, 0, 0));
    }
    X = random(matrix.width());
    Y = random(matrix.height());
    if (multi == 1) getRandom();
    if (multi == 2)
    {
      j++;
      if (j > 255) j = 0;
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void matrixRainbow(uint8_t frame, uint8_t scaler, unsigned long interval)
{
  matrix.fillScreen(Wheel(j));
  if (frame == 1) matrix.drawBitmap(0, -30, sunRise, 30, 90, (matrix.Color(0, 0, 0)));
  if (frame == 2) matrix.drawBitmap(0, y - matrix.height(), checkerOne, 30, 33, (matrix.Color(0, 0, 0)));
  if (frame == 3) matrix.drawBitmap(0, y - matrix.height(), checkerTwo, 30, 36, (matrix.Color(0, 0, 0)));
  if (frame == 4) matrix.drawBitmap(0, y - matrix.height() + 1, checkerThree, 30, 33, (matrix.Color(0, 0, 0)));
  if (frame == 5) matrix.drawBitmap(0, y - matrix.height(), burst, 30, 48, (matrix.Color(0, 0, 0)));
  if (frame == 6) matrix.drawBitmap(0, 0, radio, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 7) matrix.drawBitmap(0, 0, radioFrame, 30, 30, (matrix.Color(0, 0, 0)));
  matrix.show();

  currentMillis = millis();
  if (currentMillis - previousMillis >= (interval))
  {
    previousMillis = currentMillis;
    j = j + scaler;
    y--;
    if (frame == 2) matrix.fillScreen(0);
    if (frame == 2) if (y <= matrix.height() - 3) y = matrix.height();
    if (frame == 3) if (y <= matrix.height() - 5) y = matrix.height();
    if (frame == 4) if (y <= matrix.height() - 6) y = matrix.height();
    if (frame == 5) if (y <= matrix.height() - 25) y = matrix.height();
  }
  if (j >= 255) j = 0;
}

void fadeUp(uint8_t multi, uint8_t frame, unsigned long interval)
{
  //do this
  if (multi == 0 || multi == 1)
  {
    red = map(i, 0, 255, 0, R);
    green = map(i, 0, 255, 0, G);
    blue = map(i, 0, 255, 0, B);
    matrix.fillScreen(matrix.Color(red, green, blue));
  }
  if (multi == 2) matrix.fillScreen(map(i, 0, 255, 0, Wheel(j)));

  if (frame == 1) matrix.drawBitmap(0, 0, sunOne, 30, 30, (matrix.Color(255, 255, 0)));
  if (frame == 2)
  {
    Y = map(i, 0, 255, matrix.height(), -34);
    matrix.drawBitmap(0, Y, sunOne, 30, 30, (matrix.Color(255, 255, 0)));
  }
  if (frame == 3)
  {
    matrix.drawBitmap(0, 0, heartOneFrame, 30, 30, (matrix.Color(0, 0, 0)));
  }

  matrix.show();

  //then check the time and update variables
  currentMillis = millis();
  if (currentMillis - previousMillis >= (interval / 255))
  {
    previousMillis = currentMillis;
    i++;
    if (i > 255)
    {
      i = 0;
      if (multi == 1) getRandom();
      if (multi == 2)
      {
        j++;
        if (j > 255) j = 0;
      }
    }
  }
}

void randomRectangle(uint8_t multi, uint8_t frame, bool blackOut, bool fat, uint8_t scaler, unsigned long interval)
{
  //do this
  if (blackOut == true)
  {
    matrix.drawRect(blackX, blackY, blackXA, blackYA, matrix.Color(0, 0, 0));
    if (fat == true) matrix.drawRect(blackX - 1, blackY - 1, blackXA + 2, blackYA + 2, matrix.Color(0, 0, 0));
  }

  if (multi == 0 || multi == 1)
  {
    matrix.drawRect(X, Y, XA, YA, (matrix.Color(R, G, B)));
    if (fat == true) matrix.drawRect(X - 1, Y - 1, XA + 2, YA + 2, (matrix.Color(R, G, B)));
  }
  if (multi == 2)
  {
    matrix.drawRect(X, Y, XA, YA, (Wheel(j)));
    if (fat == true) matrix.drawRect(X - 1, Y - 1, XA + 2, YA + 2, (Wheel(j)));
  }

  if (frame == 1) matrix.drawBitmap(0, 0, sunOne, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 2) matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 3) matrix.drawBitmap(0, 0, checkerOne, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 4) matrix.drawBitmap(0, 0, checkerTwo, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 5) matrix.drawBitmap(0, 0, checkerThree, 30, 30, (matrix.Color(0, 0, 0)));

  matrix.show();

  //check the time and advance the variables
  currentMillis = millis();
  if (currentMillis - previousMillis >= (interval))
  {
    previousMillis = currentMillis;

    getRectangleRandom();

    if (multi == 1) getRandom();
    if (multi == 2)
    {
      j = j + scaler;
      if (j > 255) j = 0;
    }
    if (blackOut == true) getBlackRandom();
  }
}

void randomPlaid(uint8_t multi, uint8_t frame, bool blackOut, bool fat, uint8_t scaler, unsigned long interval)
{
  //do this
  if (blackOut == true)
  {
    // drawFastVLine(uint16_t x0, uint16_t y0, uint16_t length, uint16_t color);
    // drawFastHLine(uint8_t x0, uint8_t y0, uint8_t length, uint16_t color);
    matrix.drawFastVLine(blackX, 0, matrix.height(), matrix.Color(0, 0, 0));
    matrix.drawFastHLine(0, blackY, matrix.width(), matrix.Color(0, 0, 0));
    if (fat == true)

    {
      matrix.drawFastVLine(blackX - 1, 0, matrix.height(), matrix.Color(0, 0, 0));
      matrix.drawFastHLine(0, blackY - 1, matrix.width(), matrix.Color(0, 0, 0));
    }
  }

  if (multi == 0 || multi == 1)
  {
    matrix.drawFastVLine(X, 0, matrix.height(), matrix.Color(R, G, B));
    matrix.drawFastHLine(0, Y, matrix.width(), matrix.Color(R, G, B));
    if (fat == true)
    {
      matrix.drawFastVLine(X - 1, 0, matrix.height(), matrix.Color(R, G, B));
      matrix.drawFastHLine(0, Y - 1, matrix.width(), matrix.Color(R, G, B));
    }
  }
  if (multi == 2)
  {
    matrix.drawFastVLine(X, 0, matrix.height(), Wheel(j));
    matrix.drawFastHLine(0, Y, matrix.width(), Wheel(j));
    if (fat == true)
    {
      matrix.drawFastVLine(X - 1, 0, matrix.height(), Wheel(j));
      matrix.drawFastHLine(0, Y - 1, matrix.width(), Wheel(j));
    }
  }

  if (frame == 1) matrix.drawBitmap(0, 0, sunOne, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 2) matrix.drawBitmap(0, 0, heartFrame, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 3) matrix.drawBitmap(0, 0, checkerOne, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 4) matrix.drawBitmap(0, 0, checkerTwo, 30, 30, (matrix.Color(0, 0, 0)));
  if (frame == 5) matrix.drawBitmap(0, 0, checkerThree, 30, 30, (matrix.Color(0, 0, 0)));

  matrix.show();

  //check the time and advance the variables
  currentMillis = millis();
  if (currentMillis - previousMillis >= (interval))
  {
    previousMillis = currentMillis;

    getRectangleRandom();

    if (multi == 1) getRandom();
    if (multi == 2)
    {
      j = j + scaler;
      if (j > 255) j = 0;
    }
    if (blackOut == true) getBlackRandom();
  }
}









