//POST GRAMMERCY 2019 VERSION FOR JUMP INTO THE LIGHT

//CLOSED OTHER MENUES BUT LEFT CODE INTACT
//changed menu dot placement

//still using weird fix for unexplained non compiling

//radio remote controller with oled display and trellis keypad
//higher transmition power used for greater reliability
//multiple menus of 16 buttons
//advanced version has specific keystrokes sent depending on button pressed
//sends numerical values of ascii characters

//modified from https://learn.adafruit.com/remote-effects-trigger/overview-1?view=all
//Ada_remoteFXTrigger_TX
//Remote Effects Trigger Box Transmitter
//by John Park
//for Adafruit Industries
//MIT License

//GRAMERCY 2019 version

//written by David Crittenden 7/2018
//updated 3/2019

//BUTTON LAYOUT
/*
  row 1: ALWAYS AWAY / EVERYTHING IS BREAUTIFUL / INFERNO / ALL AROUND YOU
  row 2: RADIO GAGA / BEAUTIFUL / SOMETHING / SOMETHING ELSE
  row 3: JUMP INTO THE LIGHT / HEART FRAME FADE / Circle Droplet / Multi - Color Shock Wave
  row 4: FUCK TRUMP / Rainbow Checkers / Red-White Heart / CLEAR ALL
*/



#include <Keyboard.h>//library allows remote to act as a keyboard over usb
#include <SPI.h>
#include <RH_RF69.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_Trellis.h"
#include <Encoder.h>

/********* Encoder Setup ***************/
#define PIN_ENCODER_SWITCH 11
Encoder knob(10, 12);
uint8_t activeRow = 0;
long pos = -999;
long newpos;
int prevButtonState = HIGH;
bool needsRefresh = true;
bool advanced = false;
unsigned long startTime;


/********* Trellis Setup ***************/
#define MOMENTARY 0
#define LATCHING 1
#define MODE LATCHING //all Trellis buttons in latching mode
Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);
#define NUMTRELLIS 1
#define numKeys (NUMTRELLIS * 16)
#define INTPIN A2

/************ OLED Setup ***************/
Adafruit_SSD1306 oled = Adafruit_SSD1306();
#if defined(ESP8266)
#define BUTTON_A 0
#define BUTTON_B 16
#define BUTTON_C 2
#define LED      0
#elif defined(ESP32)
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14
#define LED      13
#elif defined(ARDUINO_STM32F2_FEATHER)
#define BUTTON_A PA15
#define BUTTON_B PC7
#define BUTTON_C PC5
#define LED PB5
#elif defined(TEENSYDUINO)
#define BUTTON_A 4
#define BUTTON_B 3
#define BUTTON_C 8
#define LED 13
#elif defined(ARDUINO_FEATHER52)
#define BUTTON_A 31
#define BUTTON_B 30
#define BUTTON_C 27
#define LED 17
#else // 32u4, M0, and 328p
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5
#define LED      13
#endif


/************ Radio Setup ***************/
// Can be changed to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

#if defined (__AVR_ATmega32U4__) // Feather 32u4 w/Radio
#define RFM69_CS      8
#define RFM69_INT     7
#define RFM69_RST     4
#endif

#if defined(ARDUINO_SAMD_FEATHER_M0) // Feather M0 w/Radio
#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
#endif

#if defined (__AVR_ATmega328P__)  // Feather 328P w/wing
#define RFM69_INT     3  // 
#define RFM69_CS      4  //
#define RFM69_RST     2  // "A"
#endif

#if defined(ESP32)    // ESP32 feather w/wing
#define RFM69_RST     13   // same as LED
#define RFM69_CS      33   // "B"
#define RFM69_INT     27   // "A"
#endif

//NEW CODE (wasn't choosing from the feather MO if endif)
#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

int lastButton = 17; //last button pressed for Trellis logic
char radiopacket[20];//stores information sent

int menuList[8] = {1, 2, 3, 4, 5, 6, 7, 8}; //for rotary encoder choices
int m = 0; //variable to increment through menu list
int lastTB[8] = {16, 16, 16, 16, 16, 16, 16, 16}; //array to store per-menu Trellis button

//for battery level
#define VBATPIN A7

/*******************SETUP************/
void setup()
{
  delay(500);
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  Keyboard.begin();//for keyboard emulation

  // INT pin on Trellis requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);
  trellis.begin(0x70);

  pinMode(PIN_ENCODER_SWITCH, INPUT_PULLUP);//set encoder push switch pin to input pullup

  digitalPinToInterrupt(10); //on M0, Encoder library doesn't auto set these as interrupts
  digitalPinToInterrupt(12);

  // Initialize OLED display
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  oled.setTextWrap(false);
  oled.clearDisplay();
  oled.display();
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

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
  rf69.setTxPower(20, true);//originally 14

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
                  };
  rf69.setEncryptionKey(key);

  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  // light up all the LEDs in order
  for (uint8_t i = 0; i < numKeys; i++)
  {
    trellis.setLED(i);
    trellis.writeDisplay();
    delay(25);
  }

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println("JUMP INTO");
  oled.setCursor(0, 16);
  oled.println("THE LIGHT");
  oled.display();
  delay(1200); //pause to let message be read by a human

  //check the battery level
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  measuredvbat *= 100;    // to move the decimal place
  int batteryPercent = map(measuredvbat, 330, 426, 0, 100);//operating voltage ranges from 3.3 - 4.26

  oled.clearDisplay();
  oled.setCursor(12, 0);
  oled.print("Batt ");
  oled.print(batteryPercent);
  oled.println("% ");
  oled.setCursor(0, 16);
  oled.print(measuredvbat / 100);
  oled.println(" volts  ");
  oled.display();
  delay(2000); //pause to let message be read by a human

  /*oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print("JUMP INTO");
    oled.setCursor(0, 16);
    oled.print("THE LIGHT");
    oled.display();
    delay (1200);*/

  // then turn them off
  for (uint8_t i = 0; i < numKeys; i++)
  {
    trellis.clrLED(i);
    trellis.writeDisplay();
    delay(25);
  }
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

void loop()
{
  delay(30); // 30ms delay is required, dont remove me! (Trellis)

  /*************Rotary Encoder Menu***********/

  //check the encoder knob, set the current position as origin
  //long newpos = knob.read() / 4;//divide for encoder detents

  /* // for debugging
    Serial.print("pos=");
    Serial.print(pos);
    Serial.print(", newpos=");
    Serial.println(newpos);
  */

  /*if (newpos != pos)
    {
    int diff = newpos - pos;//check the different between old and new position
    if (diff >= 1)
    {
      m++;
      m = (m + 8) % 8; //modulo to roll over the m variable through the list size
    }

    if (diff == -1)//rotating backwards
    {
      m--;
      m = (m + 8) % 8;
    }
    /* //uncomment for debugging or general curiosity
      Serial.print("Diff = ");
      Serial.print(diff);
      Serial.print("  pos= ");
      Serial.print(pos);
      Serial.print(", newpos=");
      Serial.println(newpos);
      Serial.println(menuList[m]);
  */

  // pos = newpos;

  // Serial.print("m is: ");
  //Serial.println(m);
  //clear Trellis lights
  /* for (int t = 0; t <= 16; t++) {
     trellis.clrLED(t);
     trellis.writeDisplay();
    }
    //light last saved light for current menu
    trellis.clrLED(lastTB[m]);
    trellis.setLED(lastTB[m]);
    trellis.writeDisplay();*/

  //NEW CODE
  //int p; //for drawing bullet point menu location pixels
  //int q;
  //write to the display
  oled.setCursor(0, 3);
  oled.clearDisplay();
  //NEW CODE
  m = 0;

  if (m == 0)
  {
    oled.drawLine(0, 31, 15, 31, WHITE);
    oled.drawLine(0, 30, 15, 30, WHITE);
    oled.print("PERFORMANCE");
  }
  /*
    if (m == 1)
    {
    oled.drawLine(16, 31, 31, 31, WHITE);
    oled.drawLine(16, 30, 31, 30, WHITE);
    oled.print(" NeoPixels");
    }
    if (m == 2)
    {
    oled.drawLine(32, 31, 47, 31, WHITE);
    oled.drawLine(32, 30, 47, 30, WHITE);
    oled.print(" Motors");
    }
    if (m == 3)
    {
    oled.drawLine(48, 31, 63, 31, WHITE);
    oled.drawLine(48, 30, 63, 30, WHITE);
    oled.print(" Lamps");
    }
    if (m == 4)
    {
    oled.drawLine(64, 31, 79, 31, WHITE);
    oled.drawLine(64, 30, 79, 30, WHITE);
    oled.print(" Traps");
    }
    if (m == 5)
    {
    oled.drawLine(80, 31, 95, 31, WHITE);
    oled.drawLine(80, 30, 95, 30, WHITE);
    oled.print(" Radio");
    }
    if (m == 6)
    {
    oled.drawLine(96, 31, 111, 31, WHITE);
    oled.drawLine(96, 30, 111, 30, WHITE);
    oled.print(" Doors");
    }
    if (m == 7)
    {
    oled.drawLine(112, 31, 127, 31, WHITE);
    oled.drawLine(112, 30, 127, 30, WHITE);
    oled.print(" Pyro");
    }

    oled.display();
    }
    //END*/

  // remember that the switch is active low
  int buttonState = digitalRead(PIN_ENCODER_SWITCH);
  if (buttonState == LOW)//button has been pressed
  {
    unsigned long now = millis();
    if (prevButtonState == HIGH)
    {
      prevButtonState = buttonState;
      startTime = now;
      Serial.println("button pressed");

      //send trellis selection
      Serial.print("Sending ");
      Serial.println(radiopacket[0]);
      rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
      rf69.waitPacketSent();

      //send a keyboard command to the computer depending on which button was pressed
      //NEW CODE
      if (m == 0)//sends different values for each button of each menu
      {
        switch (radiopacket[0])
        {
          case 1://ALWAYS AWAY - NO INTRO
            Keyboard.print("1");//send message to computer
            delay(20);
            Keyboard.print(" ");//send message to
            break;
          case 2://EVERYTHING IS BEAUTIFUL - NO INTRO
            Keyboard.print("2");//send message to computer
            delay(20);
            Keyboard.print(" ");//send message to
            break;
          case 3://INFERNO - NO INTRO
            Keyboard.print("3");//send message to computer
            delay(20);
            Keyboard.print(" ");//send message to
            break;
          case 4://ALL AROUND YOU - WITH 8 BEAT COUNT IN
            Keyboard.print("4");//send message to computer
            delay(20);
            Keyboard.print(" ");//send message to computer
            break;
          case 5://RADIO GAGA - NO INTRO
            Keyboard.print("5");//send message to computer
            delay(20);
            Keyboard.print(" ");//send message to
            break;
          case 6://BEAUTIFUL - NO INTRO
            Keyboard.print("6");//send message to computer
            delay(20);
            Keyboard.print(" ");//send message to
            break;
          case 16:
            Keyboard.print(" ");//send message to
            break;
          default:
            break;
        }
      }
      if (m == 1)//Second Menu
      {
        switch (radiopacket[0])

        default:
        break;
      }
      if (m == 2)//Third Menu
      {
        switch (radiopacket[0])

        default:
        break;
      }
      if (m == 3)//Fourth Menu
      {
        switch (radiopacket[0])

        default:
        break;
      }
      if (m == 4)//Fifth Menu
      {
        switch (radiopacket[0])

        default:
        break;
      }
      if (m == 5)//Sixth Menu
      {
        switch (radiopacket[0])

        default:
        break;
      }
      if (m == 6)//Seventh Menu
      {
        switch (radiopacket[0])

        default:
        break;
      }
      if (m == 7)//Eighth Menu
      {
        switch (radiopacket[0])

        default:
        break;
      }
    }
  }
  else if (buttonState == HIGH && prevButtonState == LOW)
  {
    Serial.println("button released!");
    prevButtonState = buttonState;
  }

  /*************Trellis Button Presses***********/
  if (MODE == LATCHING)
  {
    if (trellis.readSwitches()) { // If a button was just pressed or released...
      for (uint8_t i = 0; i < numKeys; i++) { // go through every button
        if (trellis.justPressed(i)) { // if it was pressed...
          //Serial.print("v"); Serial.println(i);

          if (i != lastTB[m]) {
            if (trellis.isLED(i)) {
              trellis.clrLED(i);
              lastTB[m] = i; //set the stored value for menu changes
            }
            else {
              trellis.setLED(i);
              //trellis.clrLED(lastButton);//turn off last one
              trellis.clrLED(lastTB[m]);
              lastTB[m] = i; //set the stored value for menu changes
            }
            trellis.writeDisplay();
          }

          //check the rotary encoder menu choice
          if (m == 0)//first menu item: PERFORMANCE MODE
          {
            if (i == 0)
            {
              radiopacket[0] = 1;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("ALWAYS");
              oled.setCursor(0, 16);
              oled.print("AWAY");
              oled.display();
            }
            if (i == 1)
            {
              radiopacket[0] = 2;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("EVERYTHING");
              oled.setCursor(0, 16);
              oled.print("UNDER..SUN");
              oled.display();
            }
            if (i == 2)
            {
              radiopacket[0] = 3;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("INFERNO");
              oled.display();
            }

            if (i == 3)
            {
              radiopacket[0] = 4;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("ALL AROUND");
              oled.setCursor(0, 16);
              oled.print("YOU");
              oled.display();
            }

            if (i == 4)
            {
              radiopacket[0] = 5;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("RADIO GAGA");
              oled.display();
            }

            if (i == 5)
            {
              radiopacket[0] = 6;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("BEAUTIFUL");
              oled.display();
            }
            if (i == 6)
            {
              radiopacket[0] = 7;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("SOMETHING");
              oled.display();
            }
            if (i == 7)
            {
              radiopacket[0] = 8;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("SOMETHING");
              oled.setCursor(0, 16);
              oled.print("ELSE");
              oled.display();
            }

            if (i == 8)
            {
              radiopacket[0] = 9;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Text Scroll");
              oled.setCursor(0, 16);
              oled.print("JUMP..LIGHT ");
              oled.display();
            }

            if (i == 9)
            {
              radiopacket[0] = 10;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Heart");
              oled.setCursor(0, 16);
              oled.print("Fade");
              oled.display();
            }

            if (i == 10)
            {
              radiopacket[0] = 11;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Circle");
              oled.setCursor(0, 16);
              oled.print("Droplet");
              oled.display();
            }

            if (i == 11)
            {
              radiopacket[0] = 12;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Multi-Color");
              oled.setCursor(0, 16);
              oled.print("Shock Wave");
              oled.display();
            }

            if (i == 12)
            {
              radiopacket[0] = 13;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Text Scroll");
              oled.setCursor(0, 16);
              oled.print("FUCK TRUMP");
              oled.display();
            }

            if (i == 13)
            {
              radiopacket[0] = 14;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Rainbow");
              oled.setCursor(0, 16);
              oled.print("Checkers");
              oled.display();
            }

            if (i == 14)
            {
              radiopacket[0] = 15;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Red-White");
              oled.setCursor(0, 16);
              oled.print("Heart");
              oled.display();
            }

            if (i == 15)
            {
              radiopacket[0] = 16;
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("CLEAR");
              oled.setCursor(0, 16);
              oled.print("ALL");
              oled.display();
            }
          }
          //NEW CODE
          /**************Motor Props**************/
          if (m == 2) { //next menu item
            if (i == 0) { //button 0 sends button D command CARD UP
              radiopacket[0] = 'H';
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Motors");
              oled.setCursor(0, 16);
              oled.print("Card....UP");
              oled.display();
            }
            if (i == 1) { //button 1 sends button E command CARD DOWN
              radiopacket[0] = 'I';
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Motors");
              oled.setCursor(0, 16);
              oled.print("Card..DOWN");
              oled.display();
            }
            if (i == 4) { //button 4 sends button F command PUMP RUN temp
              radiopacket[0] = 'J';
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Motors");
              oled.setCursor(0, 16);
              oled.print("Pump...RUN");
              oled.display();
            }
            if (i >= 5 && i <= 15) {
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Motors");

              oled.display();
            }
          }
          /**************Lamps**************/
          if (m == 3) { //next menu item
            if (i == 0) {
              radiopacket[0] = 'K';
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Lamp");
              oled.setCursor(0, 16);
              oled.print("Spot1...ON");
              oled.display();
            }
            if (i == 1) {
              radiopacket[0] = 'L';
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Lamp");
              oled.setCursor(0, 16);
              oled.print("Spot1..Off");
              oled.display();
            }
            if (i >= 2 && i <= 15) {
              oled.clearDisplay();
              oled.setCursor(0, 0);
              oled.print("Lamp");
              oled.display();
            }
          }
          //END
        }
      }
    }
  }
}

