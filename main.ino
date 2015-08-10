#include <DHT.h>

#include <VirtualWire.h>



#include <7segment.h>
#include <font5x7.h>
#include <font8x16.h>
#include <fontlargenumber.h>
#include <MicroView.h>
#include <space01.h>
#include <space02.h>
#include <space03.h>

// Temp
#define DHTPIN A5     // what pin we're connected to

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);

  float h;  
  float t; 
  float f,hi;
// Relay variables
const int relayPin = 6; 

// Button variables
const int buttonPin   = 5;    // the number of the pushbutton pin
int       buttonState = 0;    // variable for reading the pushbutton status

// WiFi enable pin
const int WifiPin = 3;

// RFID
//#define MAX_ID 20
//char      cur_id         [MAX_ID] = "";
//char      CurrentReadTag [MAX_ID] = "";
//char      inByte;
//int       len;


byte   TagRead;
byte   PreviousTagRead;
String strReadTag    = "";
String strCurrentTag = "";
bool   bTagOk        = false;
bool   bTagRead      = false; 
float  celsius       = 0.0;
// DEBUG
const bool debug = true;

void setup()
{
  Serial.begin(9600);
  if (debug) Serial.println("Setup - Start");
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT); 
  pinMode(WifiPin, INPUT); 
  pinMode(DHTPIN, INPUT); 
  
  uView.begin();			// start MicroView
  uView.clear(PAGE);			// clear page
  uView.setFontType(1);
  uView.setCursor(0,15);
  uView.print("Doorino");	
  uView.display();
  delay(2000);
  uView.clear(PAGE);			// clear page
  uView.display();
  
  dht.begin();
  delay(500);
}


void loop()
{
  byte i         = 0;
  byte val       = 0;
  byte checksum  = 0;
  byte bytesread = 0;
  byte tempbyte  = 0;
  byte code[6];
  strReadTag     = "";
  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius
  t = dht.readTemperature();
  
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  

  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.println(" *C ");
  
  Serial.print("read pin:");
  Serial.println(digitalRead(WifiPin));
  
  
  // Button pushed?
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) ButtonPushed();

  if (digitalRead(WifiPin) == HIGH) WiFiOpenDoor();
  
  if(Serial.available() > 0) {
    if((val = Serial.read()) == 2) {                  // check for header 
      bytesread = 0; 
      while (bytesread < 12) {                        // read 10 digit code + 2 digit checksum
        if( Serial.available() > 0) { 
          val = Serial.read();
          if((val == 0x0D)||(val == 0x0A)||(val == 0x03)||(val == 0x02)) { // if header or stop bytes before the 10 digit reading 
            break;                                                         // stop reading
          }
          // Do Ascii/Hex conversion:
          if ((val >= '0') && (val <= '9')) {
            val = val - '0';
          } else if ((val >= 'A') && (val <= 'F')) {
            val = 10 + val - 'A';
          }

          // Every two hex-digits, add byte to code:
          if (bytesread & 1 == 1) {
            // make some space for this hex-digit by
            // shifting the previous hex-digit with 4 bits to the left:
            code[bytesread >> 1] = (val | (tempbyte << 4));

            if (bytesread >> 1 != 5) {                // If we're at the checksum byte,
              checksum ^= code[bytesread >> 1];       // Calculate the checksum... (XOR)
            };
          } else {
            tempbyte = val;                           // Store the first hex digit first...
          };

          bytesread++;                                // ready to read next digit
        } 
      } 

      // Output to Serial:

      if (bytesread == 12) {                          // if 12 digit read is complete
        Serial.print("5-byte code: ");
        for (i=0; i<5; i++) 
        {
          if (code[i] < 16)
         {
           Serial.print("0");
           strReadTag = strReadTag + "0";
         }
         
          
          Serial.print(code[i], HEX);
          strReadTag = strReadTag + String(code[i],HEX);
          Serial.print(" ");
        }
        // TO UPPER CASE
        strReadTag.toUpperCase();
        
        Serial.println();

        Serial.print("Checksum: ");
        Serial.print(code[5], HEX);
        Serial.println(code[5] == checksum ? " -- passed." : " -- error.");
        Serial.println();
        Serial.print("strReadTag: ");
        Serial.println(strReadTag);
      }

      bytesread = 0;
    }
  }

  if ( strReadTag == "54003846CF" || strReadTag == "0415EDADC4" || strReadTag == "51007B9710" )
    { OpenDoor(); }
    else if (strReadTag != "")
    {
      uView.clear(PAGE);
      uView.setCursor(0,10);			// clear page
      uView.print("Unknown");
      uView.setCursor(0,25);			// clear page
      uView.print("id");
      uView.display();
      delay(2000);
      uView.clear(PAGE);			// clear page
      uView.display();
    }
    delay(300);
}
 
/* RELAY FUNCTIONS */
void OpenDoor()
{
  String strName = "";
  if (strReadTag == "54003846CF") strName = "Ole Ivar";
  if (strReadTag == "51007B9710") strName = "Camilla";
  Serial.println("Door open");
  uView.clear(PAGE);
  uView.setFontType(0);
  uView.setCursor(0,10);			// clear page
  uView.print("Velkommen");
  uView.setCursor(0,20);			// clear page
  uView.print(strName);
  uView.setFontType(0);
  uView.setCursor(20,40);			// clear page
  uView.print((float)t,2);
  uView.print("/");
  uView.print((float)h,2);

  uView.display();

  digitalWrite(relayPin, HIGH); 
  delay(2000);                  // waits for 2 seconds
  Serial.println("Door closed");

  uView.clear(PAGE);
  uView.setCursor(0,10);			// clear page
  uView.print("Door");
  uView.setCursor(0,30);			// clear page
  uView.print("closed");
  uView.display();

  digitalWrite(relayPin, LOW);  
//  delay(2000);
  uView.clear(PAGE);			// clear page
  uView.display();
}
void ButtonPushed()
{
  Serial.println("Button pushed");
  uView.clear(PAGE);
  uView.setFontType(1);
  uView.setCursor(10,10);			// clear page
  uView.print("Bye");
  uView.setFontType(0);
  uView.setCursor(0,40);
  uView.print((float)t,2);
  uView.print("/");
  uView.print((float)h,2);


  uView.display();

  digitalWrite(relayPin, HIGH); 
  delay(2000);                  // waits for 2 seconds
  digitalWrite(relayPin, LOW);  

  uView.clear(PAGE);			// clear page
  uView.display();

}

void WiFiOpenDoor()
{
  Serial.println("Open door signal from WiFi");
  uView.clear(PAGE);
  uView.setFontType(1);
  uView.setCursor(10,10);			// clear page
  uView.print("Hurry!");
  uView.setFontType(0);
  uView.setCursor(0,40);
  uView.print((float)t,2);
  uView.print("/");
  uView.print((float)h,2);


  uView.display();

  digitalWrite(relayPin, HIGH); 
  delay(30000);                  // waits for 2 seconds
  digitalWrite(relayPin, LOW);  

  uView.clear(PAGE);			// clear page
  uView.display();

}


