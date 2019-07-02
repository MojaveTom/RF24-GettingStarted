
/*
* Getting Started example sketch for nRF24L01+ radios
* This is a very basic example of how to send data from one node to another
* Updated: Dec 2014 by TMRh20
*/

#include <arduino.h>
#include <SPI.h>
#include "RF24.h"

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = RADIO_NUMBER;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus on pins defined in platformio.ini */
RF24 radio(CE_PIN, CSN_PIN);
/**********************************************************/

byte addresses[][6] = {"1Node","2Node"};

// Used to control whether this node is sending or receiving
bool role = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  // printf_begin();
  Serial.println(F("RF24/examples/GettingStarted"));
  Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));

//  radio.printDetails();

  radio.begin();

  Serial.println(radio.isChipConnected()? F("Chip connected.") : F("Chip NOT connected."));
  Serial.println(radio.isValid()? F("Radio valid.") : F("Radio NOT valid."));
  Serial.println(radio.isPVariant()? F("Radio is P variant.") : F("Radio NOT P variant."));

  Serial.print("txDelay = "); Serial.println(radio.txDelay);
  Serial.print("csDelay = "); Serial.println(radio.csDelay);

  //  radio.printDetails();

  radio.setChannel(RADIO_CHANNEL);

  // Set the PA Level low to prevent power supply related issues since this is a
 // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_MAX);

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
}

void loop() {

static double sampleCount = 0, mean = 0, variance = -1, t = 0, mint, maxt, failCount = 0;
static uint32_t waitTime = 200;

/****************** Ping Out Role ***************************/
if (role == 1)  {
    radio.stopListening();                                    // First, stop listening so we can talk.


    Serial.print(F("Now sending, waitTime = ")); Serial.println(radio.waitDelay);

    unsigned long start_time = micros();                             // Take the time, and send it.  This will block until complete
     if (!(t = radio.write( &start_time, sizeof(unsigned long) ))){
       Serial.println(F("Write failed."));
       failCount++;
     }

    radio.startListening();                                    // Now, continue listening

    if (t > 0)
    {    sampleCount += 1;
        if (sampleCount == 1) {
          mean = t;
          variance = 0;
          mint = maxt = t;
        }
        else {
          if (t < mint) mint = t;
          if (t > maxt) maxt = t;
          double mm1 = mean;
          mean = mean + (t - mean)/sampleCount;
          variance = variance + (t - mm1)*(t - mean);
        }

        Serial.print("Fail count = "); Serial.print(failCount);
        Serial.print("; this delay = "); Serial.print(t); Serial.print("; N = "); Serial.print(sampleCount);
        Serial.print("; min delay = "); Serial.print(mint); Serial.print("; max delay = "); Serial.print(maxt);
        Serial.print("; avg delay = "); Serial.print(mean); Serial.print("; std dev = "); Serial.println(sqrt(variance/(sampleCount-1)));
    }
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
        Serial.print(end_time - start_time);
        Serial.println(F(" microseconds"));
    }

    // Try again 1s later
    delay(1000);
  }



/****************** Pong Back Role ***************************/

  if ( role == 0 )
  {
    unsigned long got_time;

    if( radio.available()){
                                                                    // Variable for the received timestamp
      while (radio.available()) {                                   // While there is data ready
        radio.read( &got_time, sizeof(unsigned long) );             // Get the payload
      }

      radio.stopListening();                                        // First, stop listening so we can talk
      radio.write( &got_time, sizeof(unsigned long), 0 );              // Send the final one back.
      radio.startListening();                                       // Now, resume listening so we catch the next packets.
      Serial.print(F("Sent response "));
      Serial.println(got_time);
   }
 }




/****************** Change Roles via Serial Commands ***************************/

  if ( Serial.available() )
  {
    static char numbers[20];
    static uint8_t numCnt = 0;
    char c = toupper(Serial.read());
    Serial.print(c);
    if ( c == 'T' && role == 0 ){
      Serial.println(F("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK"));
      role = 1;                  // Become the primary transmitter (ping out)
      sampleCount = 0;
      failCount = 0;

   }else
    if ( c == 'R' && role == 1 ){
      Serial.println(F("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK"));
       role = 0;                // Become the primary receiver (pong back)
       radio.startListening();

    } else
      if (( c <= '9') && (c >= '0')) {
        numbers[numCnt] = c;
        numCnt++;
        if (numCnt >= sizeof(numbers)) {
          numCnt = sizeof(numbers)-1;
        }
      } else {
        if (numCnt > 0) {
          Serial.print("0x"); if (c < 0x10) Serial.print("0"); Serial.println(c, HEX);
          numbers[numCnt] = '\0';
          Serial.print("numbers are: "); Serial.println(numbers);
          if (( c == '\n') || ( c == '\r')) {
            waitTime = 0;
            for ( int i = 0; i<numCnt; i++) {
              waitTime = waitTime * 10 + (numbers[i] - '0');
            }
            numCnt = 0;
            Serial.print("Setting waitTime to "); Serial.println(waitTime);
            radio.waitDelay = waitTime;
            }
          }
       }
  }

} // Loop
