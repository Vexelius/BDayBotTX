//  **=============================================**
//  ||                 PASTELINO                   ||
//  ++---------------------------------------------++
//  ||               Remote Control                ||
//  ||                                             ||
//  ||  Powered by:                                ||
//  ||    -ICStation ATmega328 Pro Mini [5V,16Mhz] ||
//  ||    -nRFL2401 module                         ||
//  ||                                             ||
//  **=============================================**

#include <SPI.h>
#include "RF24.h" //Provided by TMRh20 : http://tmrh20.github.io/RF24
#include <printf.h>
#include <LiquidCrystal.h>
#include <OnewireKeypad.h>  //By HazardsMind : http://playground.arduino.cc/Code/OneWireKeyPad

// Keypad configuration
#define Rows 2
#define Cols 5
#define Pin A0
#define Row_Res 4600
#define Col_Res 995

char KEYS[]= {
  'U','R','D','L','S',
  'C','B','A','M','Z',
};
// Key codes:
// U: Up, D: Down, R: Right, L: Left
// S: Select Mode - || Z: Select Mode +
// Action buttons: A, B, C
// M: Play Melody

// Initialize the LCD library with the numbers of the interface pins
LiquidCrystal lcd(9, 6, 5, 4, 3, 2);

// Initialize the Keypad library
OnewireKeypad <LiquidCrystal, 10 > Keypad(lcd, KEYS, Rows, Cols, Pin, Row_Res, Col_Res);

// Set up the NRF24L01
RF24 radio(7,8);

byte addresses[][6] = {"1Node", "2Node"}; // Create the pipes
unsigned long timeNow;  // Grabs the current time, used to calculate delays
unsigned long started_waiting_at;
boolean timeout;       // Timeout flag
boolean transmitData; // Enable the transmitter and send data

boolean lcdRefresh;    // Controls the LCD's refresh rate
int robotMode;  // Sets up the operation mode
// 1: Expression Mode   3: Greeting Mode
// 2: Candle Mode       5: Config Mode
int dizzyCounterL;
int dizzyCounterR;
int timeoutCounter;

int expressionIndex = 1;  //This variable will aid to display the list of available expressions

boolean statusConnect;  // Checks the connection status between control and robot

struct dataStruct {
  unsigned long timeCounter;  // Save response times
  char keyPress;          // When a key is pressed, this variable stores its unique code
  int keyState;           // Stores the status of the pressed key
  boolean keypadLock;     // When this flag is active, no input will be received fron the keypad
  boolean configMode;     // This flag determines wheter the robot is in Config Mode or not
  boolean statusDizzy;
  int expression;         // Used to change the robot's expressions
  boolean candleA;        // Sets the status for the first Candle
  boolean candleB;        // Sets the status for the second Candle
  boolean candleC;        // Sets the status for the third Candle
  int greetingStatus;     // This flag shows the status for the greeting | 0 = Inactive | 1 = Sent | 2 = Displayed
  char greeting1[10]="Arduino!";     // The first message
} myData;                 // Data stream that will be sent to the robot

  //Sprite for the lit candle
  byte candleOn[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b10001,
  0b01010,
  0b00100
  };

  //Sprite for the unlit candle
  byte candleOff[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00100,
  0b00110,
  0b00010,
  0b00110,
  0b00100
  };

  byte candleBody[8] = {
  0b11111,
  0b10001,
  0b11101,
  0b10101,
  0b10011,
  0b10001,
  0b10001,
  0b10001
  };

void setup() {
  Serial.begin(57600);
  // Set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print the welcome message
  lcd.print("Pastelino");
  lcd.setCursor(8,1);
  lcd.print("Ver. 0.9");

  robotMode = 1;
  myData.configMode = false;
  myData.statusDizzy = false;
  Keypad.SetHoldTime(800);
  dizzyCounterL = 0;
  dizzyCounterR = 0;
  timeoutCounter = 0;
  statusConnect = true;

  myData.candleA = false; 
  myData.candleB = false;
  myData.candleC = false;

  lcd.createChar(0, candleOff);
  lcd.createChar(1, candleOn);
  lcd.createChar(2, candleBody);

  myData.greetingStatus = 0;

  printf_begin(); // Needed for "printDetails" Takes up some memory

  radio.begin();  //Initialize NRF24L01
  radio.setDataRate(RF24_250KBPS);  //Data rate is slow, but ensures accuracy
  radio.setPALevel(RF24_PA_HIGH);   //High PA Level, to give enough range
  radio.setCRCLength(RF24_CRC_16);  //CRC at 16 bits
  radio.setRetries(15,15);          //Max number of retries
  radio.setPayloadSize(32);          //Payload size of 32bits
  radio.setChannel(108);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);

  // Start listening for data
  radio.startListening();

  delay(1000);
  lcdRefresh = true;
}

void loop() {
  if(lcdRefresh==true)  // Update the screen when needed
  screenDraw();
  
  
  if(Keypad.Getkey())   // Look for key input
  {
  myData.keyPress = Keypad.Getkey();  // Store the key code
  myData.keyState = Keypad.Key_State();

  // Keys to change the robot's Operation Mode: S and Z
  
if(myData.configMode==false)     // Outside of Config Mode, these keys cycle
{                                // through the three main Modes
  
  if(myData.keyPress=='S')  // S Key: Select Mode -
  {
    robotMode -= 1;
    // If you press the key when you're on the first of three modes available
    // loop back to the third mode
    if(robotMode<1) robotMode = 3;
    screenDraw();  //Refresh the LCD screen's content
    delay(200);
  }
  
  if((myData.keyPress=='Z')&&(Keypad.Key_State()!=3))  // Z Key: Select Mode +
  {
    robotMode += 1;
    // If you press the key when you're on the third of three modes available
    // loop back to the first mode
    if(robotMode>3) robotMode = 1;
    screenDraw();  //Refresh the LCD screen's content
    delay(100);
  }

  if((myData.keyPress=='Z')&&(Keypad.Key_State()==3))  // Hold Z Key: Enter Config Mode
  {
    robotMode = 5;
    myData.configMode = true;
    screenDraw();  //Refresh the LCD screen's content
    transmitData = true;
  }
}
else    // To exit Config Mode, press and hold the S Key
{
  if((myData.keyPress=='S')&&(Keypad.Key_State()==3))  // Hold S Key: Exit Config Mode
  {
    robotMode = 1;
    myData.configMode = false;
    screenDraw();  //Refresh the LCD screen's content
    transmitData = true;
    delay(200);
  }
}


  //Main Controls
  // The following instructions ensure that the proper commands are sent
  // when the Robot is in a specific Operation Mode
  
  if(robotMode==1) // Expression Mode
  {                  
  myData.greetingStatus = 0;
  if((myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
  || myData.keyPress=='M' || myData.keyPress=='B'))
  // In this mode, the Robot can move in all directions (U,D,L,R)
  // change expressions (B) and play music (M)
    {
      if(myData.keyPress=='B')
      {
      myData.expression = expressionIndex;
      }
      else
      {
      myData.expression = 0;  // Return to the default expression: Blink
      }
    transmitData = true;
    }
  // The following keys aren't transmitted, as they are using to change the data
  // that will be sent
  if(myData.keyPress=='A')
    {
      expressionIndex -= 1;
      if(expressionIndex<1) expressionIndex = 11;
      screenDraw();  //Refresh the LCD screen's content
      delay(200);
    }
  if(myData.keyPress=='C')
    {
      expressionIndex += 1;
      if(expressionIndex>11) expressionIndex = 1;
      screenDraw();  //Refresh the LCD screen's content
      delay(100); 
    }
  }
  

  if(robotMode==2) // Candle Mode
  {
  myData.greetingStatus = 0;
  if((myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
  || myData.keyPress=='M' || myData.keyPress=='A' || myData.keyPress=='B' || myData.keyPress=='C'))
  // In this mode, the Robot can move in all directions (U,D,L,R)
  // turn on and off its candles (A,B,C) and play music (M)
    {
      if(myData.keyPress=='A')
      {
        myData.candleA = !myData.candleA;
        myData.expression = 6;
        screenDraw();
      }
      if(myData.keyPress=='B')
      {
        myData.candleB = !myData.candleB;
        myData.expression = 10;
        screenDraw();
      }
      if(myData.keyPress=='C')
      {
        myData.candleC = !myData.candleC;
        myData.expression = 7;
        screenDraw();
      }
      if(myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R')
      {
        myData.expression = 0;  // Return to the default expression: Blink
      }
    transmitData = true;
    }
  }
  
  if(robotMode==3) // Greeting Mode
  {
  myData.expression = 0;
  if((myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
  || myData.keyPress=='B'))
  // In this mode, the Robot can move in all directions (U,D,L,R)
  // and play different greetings (B)
    {
      if(myData.keyPress=='B')
      {
        myData.greetingStatus = 1;
        transmitData = true;
      }
      else
      {
        myData.greetingStatus = 0;
        transmitData = true;
      }
    }
  }


  //Dizziness
  // When the Robot spins too much in one direction
  // it becomes Dizzy for a while
  if(myData.keyPress=='L')
    dizzyCounterL += 1;     // This counter keeps track of how much the Robot spins
    else                    // to the left
      dizzyCounterL = 0;    // If another key is pressed, the counter resets
  if(myData.keyPress=='R')
    dizzyCounterR += 1;     // This counter keeps track of how much the Robot spins
    else                    // to the right
      dizzyCounterR = 0;    // If another key is pressed, the counter resets

  // After the Robot has spun over this threshold...
  if((dizzyCounterL >= 5)||(dizzyCounterR >= 5))
  {
    myData.statusDizzy = true;  //... He becomes Dizzy
    sendData();
    Serial.println("I am feeling dizzy...@_@");
    delay(5000);    // He won't obey commands for a while
    // Afterwards, he'll recover and his counters will be reset
    myData.statusDizzy = false;
    dizzyCounterL = 0;
    dizzyCounterR = 0;
    sendData();
    delay(300);   // Delay to ensure that no key input is accepted for a moment after he recovers
                  // allowing him to end the Dizzy event
  }
  
    
  if(transmitData==true) // Turn on the transmitter only when there's a key press
  {
    sendData();
  }
  delay(200); // This allows for the key to debounce. Without it, there's a chance that a wrong keycode will be sent
  if(Keypad.Key_State()==2) // In the meantime, has the button been released?
    {
      if((robotMode==1) // Check if the button released is valid for Expression Mode
      &&(myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
      || myData.keyPress=='M' || myData.keyPress=='B'))
      {
        myData.keyState=2;  // If so, update the keyState variable
        sendData();         // and tell the robot about the change
      }

      if((robotMode==2) // Check if the button released is valid for Candle Mode
      &&(myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
      || myData.keyPress=='M' || myData.keyPress=='A' || myData.keyPress=='B' || myData.keyPress=='C'))
      {
        myData.keyState=2;  // If so, update the keyState variable
        sendData();         // and tell the robot about the change
      }

      if((robotMode==3) // Greeting Mode
      &&(myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
      || myData.keyPress=='B'))
      {
        myData.keyState=2;  // If so, update the keyState variable
        sendData();         // and tell the robot about the change
      }
    }
  }
}


void screenDraw() {
  lcd.clear();

  if(robotMode==1)
  {
    lcd.print("*Set Expression*");
    if(expressionIndex==1)
    {
    lcd.setCursor(5,1);
    lcd.print("[^_^]");
    }
    if(expressionIndex==2)
    {
    lcd.setCursor(5,1);
    lcd.print("[O_O]");
    }
    if(expressionIndex==3)
    {
    lcd.setCursor(5,1);
    lcd.print("[ <3 ]");
    }
    if(expressionIndex==4)
    {
    lcd.setCursor(5,1);
    lcd.print("[@_@]");
    }
    if(expressionIndex==5)
    {
    lcd.setCursor(5,1);
    lcd.print("[0v0]");
    }
    if(expressionIndex==6)
    {
    lcd.setCursor(5,1);
    lcd.print("[- 0]");
    }
    if(expressionIndex==7)
    {
    lcd.setCursor(5,1);
    lcd.print("[0 -]");
    }
    if(expressionIndex==8)
    {
    lcd.setCursor(5,1);
    lcd.print("[U_U]");
    }
    if(expressionIndex==9)
    {
    lcd.setCursor(5,1);
    lcd.print("[>.<]");
    }
    if(expressionIndex==10)
    {
    lcd.setCursor(5,1);
    lcd.print("[Yes]");
    }
    if(expressionIndex==11)
    {
    lcd.setCursor(5,1);
    lcd.print("[No-]");
    }
  }
  if(robotMode==2)
  {
    lcd.setCursor(1,0);
    if(myData.candleA==true)
    lcd.write(byte(1));
    else
    lcd.write(byte(0));

    lcd.setCursor(8,0);
    if(myData.candleB==true)
    lcd.write(byte(1));
    else
    lcd.write(byte(0));

     lcd.setCursor(15,0);
    if(myData.candleC==true)
    lcd.write(byte(1));
    else
    lcd.write(byte(0));
    
    lcd.setCursor(0,1);
    lcd.print("a");
    lcd.write(byte(2));

    lcd.setCursor(7,1);
    lcd.print("b");
    lcd.write(byte(2));

    lcd.setCursor(14,1);
    lcd.print("c");
    lcd.write(byte(2));
  }
  if(robotMode==3)
  {
    lcd.print("Greeting Mode");
  }
  if(robotMode==5)
  {
    lcd.print("Config Mode");
  }

  if(statusConnect==false)
  {
    lcd.clear();
    lcd.print("No connection...");
    lcd.setCursor(4,2);
    lcd.print("[ U_U ]");
  }
  
  lcdRefresh = false;
}


// This function controls the NRF24l01, to send commands and data to the Robot
void sendData() {
  radio.stopListening();
  myData.timeCounter = micros();  // Send back for timing

  Serial.print(F("Now sending  -  "));

  if (!radio.write( &myData, sizeof(myData) )) {            // Send data, checking for error ("!" means NOT)
    Serial.println(F("Transmit failed "));
  }

  radio.startListening();                                    // Now, continue listening

  started_waiting_at = micros();               // timeout period, get the current microseconds
  timeout = false;                            //  variable to indicate if a response was received or not

  while ( ! radio.available() ) {                            // While nothing is received
    if (micros() - started_waiting_at > 200000 ) {           // If waited longer than 200ms, indicate timeout and exit while loop
      timeout = true;
      break;
    }
  }

  if ( timeout )
  { // Describe the results
    Serial.println(F("Response timed out -  no Acknowledge."));
    timeoutCounter++; //Increase the timeout error counter
    
    if(timeoutCounter >= 100)
    statusConnect = false;
    lcdRefresh = true;
  }
  else
  {
    timeoutCounter=0; //Reset the timeout error counter
    if(statusConnect==false)
    {
      statusConnect = true; //Connection successful
      lcdRefresh = true;
    }
    
    // Grab the response, compare, and send to Serial Monitor
    radio.read( &myData, sizeof(myData) );
    timeNow = micros();

    // Show it
    Serial.print(F("Sent "));
    Serial.print(myData.keyPress);
    Serial.print(myData.keyState);
    Serial.print(" - ");
    Serial.print(F(", Got response "));
    Serial.print(myData.timeCounter);
    Serial.print(F(", Round-trip delay "));
    Serial.print(timeNow - myData.timeCounter);
    Serial.println(F(" microseconds "));

  }

  // Send again after delay. When working OK, change to something like 100
  transmitData = false;
  delay(100); // Changing this value also changes the "feeling" when you press the button. 
              // Less time = More fluid reaction
}

