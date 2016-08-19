
#include <SPI.h>
#include "RF24.h"
#include <printf.h>
#include <LiquidCrystal.h>
#include <OnewireKeypad.h>

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
// 2: Candle Mode

struct dataStruct {
  unsigned long timeCounter;  // Save response times
  char keyPress;          // When a key is pressed, this variable stores its unique code
  boolean keypadLock;     // When this flag is active, no input will be received fron the keypad
  boolean configMode;     // This flag determines wheter the robot is in Config Mode or not
} myData;                 // Data stream that will be sent to the robot

void setup() {
  Serial.begin(57600);
  // Set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print the welcome message
  lcd.print("B-DayBot");
  lcd.setCursor(8,2);
  lcd.print("Ver. 0.2");

  robotMode = 1;
  lcdRefresh = true;

  printf_begin(); // Needed for "printDetails" Takes up some memory

  radio.begin();  //Initialize NRF24L01
  radio.setDataRate(RF24_250KBPS);  //Data rate is slow, but ensures accuracy
  radio.setPALevel(RF24_PA_HIGH);   //High PA Level, to give enough range
  radio.setCRCLength(RF24_CRC_16);  //CRC at 16 bits
  radio.setRetries(15,15);          //Max number of retries
  radio.setPayloadSize(8);          //Payload size of 8bits

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);

  // Start the radio listening for data
  radio.startListening();
}

void loop() {
  if(lcdRefresh==true)
  screenDraw();
  
  
  if(Keypad.Getkey())   // Only start the process when a key is pressed
  {
  myData.keyPress = Keypad.Getkey();  // Store the key code

  // Keys to change the robot's operation mode: S and Z
  if(myData.keyPress=='S')  // S Key: Select Mode -
  {
    robotMode -= 1;
    
    // If you press the key when you're on the first of three modes available
    // loop back to the third mode
    if(robotMode<1) robotMode = 3;
    lcdRefresh = true;  //Refresh the LCD screen's content
  }
  
  if(myData.keyPress=='Z')  // Z Key: Select Mode +
  {
    robotMode += 1;
    
    // If you press the key when you're on the third of three modes available
    // loop back to the first mode
    if(robotMode>3) robotMode = 1;
    lcdRefresh = true;  //Refresh the LCD screen's content
  }

  
  if((robotMode==1)
  &&(myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
  || myData.keyPress=='M' || myData.keyPress=='B'))
  transmitData = true;

  if((robotMode==2)
  &&(myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
  || myData.keyPress=='M' || myData.keyPress=='A' || myData.keyPress=='B' || myData.keyPress=='C'))
  transmitData = true;

  if((robotMode==3)
  &&(myData.keyPress=='U' || myData.keyPress=='D' || myData.keyPress=='L' || myData.keyPress=='R' 
  || myData.keyPress=='B'))
  transmitData = true;
  
    
  if(transmitData==true) // Turn on the transmitter only when there's a key press
  {
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
  }
  else
  {
    // Grab the response, compare, and send to Serial Monitor
    radio.read( &myData, sizeof(myData) );
    timeNow = micros();

    // Show it
    Serial.print(F("Sent "));
    Serial.print(timeNow);
    Serial.print(F(", Got response "));
    Serial.print(myData.timeCounter);
    Serial.print(F(", Round-trip delay "));
    Serial.print(timeNow - myData.timeCounter);
    Serial.println(F(" microseconds "));

  }

  // Send again after delay. When working OK, change to something like 100
  transmitData = false;
  delay(200);
  }
  delay(200); // This allows for the key to debounce. Without it, there's a chance that a wrong keycode will be sent
  }
}


void screenDraw() {
  lcd.clear();

  if(robotMode==1)
  {
    lcd.print("Expression Mode");
  }
  if(robotMode==2)
  {
    lcd.print("Candle Mode");
  }
  if(robotMode==3)
  {
    lcd.print("Greeting Mode");
  }
  
  lcdRefresh = false;
}

