
// include the library code:
#include <LiquidCrystal.h>
#include <OnewireKeypad.h>

#define Rows 2
#define Cols 5
#define Pin A0
#define Row_Res 4600
#define Col_Res 995

char KEYS[]= {
  'U','R','D','L','S',
  'C','B','A','M','Z',
};

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9, 6, 5, 4, 3, 2);

OnewireKeypad <LiquidCrystal, 10 > Keypad(lcd, KEYS, Rows, Cols, Pin, Row_Res, Col_Res);

void setup() {
  Serial.begin(57600);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Key matrix test");
}

void loop() {
  // Turn off the blinking cursor:
  //lcd.noBlink();
  //delay(3000);
  // Turn on the blinking cursor:
  //lcd.blink();
  //delay(3000);
  
    if(Keypad.Getkey() )
  {
    lcd.clear();
    lcd.print("Key press: ");
    lcd.print(Keypad.Getkey());
    delay(250);
  } 
}




/*
#include <SPI.h>
#include "RF24.h"
#include <printf.h>
#include <LiquidCrystal.h>


bool radioNumber = 0;

RF24 radio(7,8);
LiquidCrystal lcd(9, 6, 5, 4, 3, 2);


byte addresses[][6] = {"1Node","2Node"};

// Used to control whether this node is sending or receiving
bool role = 0;

void setup() {
  Serial.begin(57600);
  printf_begin();
  Serial.println(F("RF24/examples/GettingStarted"));
  Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));
  
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
 // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
 radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setCRCLength(RF24_CRC_16);
  radio.setRetries(15,15);
  radio.setPayloadSize(8);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }

  
  // Start the radio listening for data
  radio.startListening();
  radio.printDetails();

    // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Transmitting!");
}

void loop() {
  
  
 
if (role == 1)  {
    
    radio.stopListening();                                    // First, stop listening so we can talk.
    
    
    Serial.println(F("Now sending"));

    unsigned long start_time = micros();                             // Take the time, and send it.  This will block until complete
     if (!radio.write( &start_time, sizeof(unsigned long) )){
       Serial.println(F("failed"));
     }
        
    radio.startListening();                                    // Now, continue listening
    
    unsigned long started_waiting_at = micros();               // Set up a timeout period, get the current microseconds
    boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not
    
    while ( ! radio.available() ){                             // While nothing is received
      if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
          timeout = true;
          break;
      }      
    }
        
    if ( timeout ){                                             // Describe the results
        Serial.println(F("Failed, response timed out."));
    }else{
        unsigned long got_time;                                 // Grab the response, compare, and send to debugging spew
        radio.read( &got_time, sizeof(unsigned long) );
        unsigned long end_time = micros();
        
        // Spew it
        Serial.print(F("Sent "));
        Serial.print(start_time);
        Serial.print(F(", Got response "));
        Serial.print(got_time);
        Serial.print(F(", Round-trip delay "));
        Serial.print(end_time-start_time);
        Serial.println(F(" microseconds"));

        lcd.clear();
        lcd.print("Success!");
        lcd.setCursor(0,1);
        lcd.print("Ping time: ");
        lcd.print(start_time);
        
    }

    // Try again 1s later
    delay(1000);
  }





  if ( role == 0 )
  {
    unsigned long got_time;
    
    if( radio.available()){
                                                                    // Variable for the received timestamp
      while (radio.available()) {                                   // While there is data ready
        radio.read( &got_time, sizeof(unsigned long) );             // Get the payload
      }
     
      radio.stopListening();                                        // First, stop listening so we can talk   
      radio.write( &got_time, sizeof(unsigned long) );              // Send the final one back.      
      radio.startListening();                                       // Now, resume listening so we catch the next packets.     
      Serial.print(F("Sent response "));
      Serial.println(got_time);  
   }
 }






  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' && role == 0 ){      
      Serial.println(F("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK"));
      role = 1;                  // Become the primary transmitter (ping out)
    
   }else
    if ( c == 'R' && role == 1 ){
      Serial.println(F("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK"));      
       role = 0;                // Become the primary receiver (pong back)
       radio.startListening();
       
    }
  }


} // Loop
*/

