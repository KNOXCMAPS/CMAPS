#include "Adafruit_FONA.h"
#include "Adafruit_FONA_3G.h"

#define FONA_TX 2
#define FONA_RX 3
#define FONA_RST 9

#ifdef __AVR__
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
#else
HardwareSerial *fonaSerial = &Serial1;
#endif

////////////  LCD I2C DISPLAY /////////////////////////
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR    0x3F // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);


// IMPORTANT "DeviceUUID" defines the unique ID. LABEL THE ARDUINO UNIT with the number after upload.
const int DeviceUUID = 502;

// Use this for FONA 800 and 808s
//Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
// Use this one for FONA 3G
Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);    
//For Auto Restart    
int RestartCount;   
// floatToString.h    
//    
// Tim Hirzel   
// tim@growdown.com   
// March 2008   
// float to string    
//    
// If you don't save this as a .h, you will want to remove the default arguments    
//     uncomment this first line, and swap it for the next.  I don't think keyword arguments compile in .pde files    
char * floatToString(char * outstr, float value, int places, int minwidth, bool rightjustify=false) {   
    // this is used to write a float value to string, outstr.  oustr is also the return value.    
    int digit;    
    float tens = 0.1;     
    int tenscount = 0;    
    int i;    
    float tempfloat = value;    
    int c = 0;    
    int charcount = 1;    
    int extra = 0;    
    // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import   
    // if this rounding step isn't here, the value  54.321 prints as 54.3209
    // calculate rounding term d:   0.5/pow(10,places)  
    float d = 0.5;
    if (value < 0)
        d *= -1.0;
    // divide by ten for each decimal place
    for (i = 0; i < places; i++)
        d/= 10.0;
    // this small addition, combined with truncation will round our values properly 
    tempfloat +=  d;

    // first get value tens to be the large power of ten less than value    
    if (value < 0)
        tempfloat *= -1.0;
    while ((tens * 10.0) <= tempfloat) {
        tens *= 10.0;
        tenscount += 1;
    }

    if (tenscount > 0)
        charcount += tenscount;
    else
        charcount += 1;

    if (value < 0)
        charcount += 1;
    charcount += 1 + places; 

    minwidth += 1; // both count the null final character
    if (minwidth > charcount){        
        extra = minwidth - charcount;
        charcount = minwidth;
    }

    if (extra > 0 and rightjustify) {
        for (int i = 0; i< extra; i++) {
            outstr[c++] = ' ';
        }
    }

    // write out the negative if needed
    if (value < 0)
        outstr[c++] = '-';

    if (tenscount == 0) 
        outstr[c++] = '0';

    for (i=0; i< tenscount; i++) {
        digit = (int) (tempfloat/tens);
        itoa(digit, &outstr[c++], 10);
        tempfloat = tempfloat - ((float)digit * tens);
        tens /= 10.0;
    }

    // if no places after decimal, stop now and return

    // otherwise, write the point and continue on
    if (places > 0)
    outstr[c++] = '.';


    // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
    for (i = 0; i < places; i++) {
        tempfloat *= 10.0; 
        digit = (int) tempfloat;
        itoa(digit, &outstr[c++], 10);
        // once written, subtract off that digit
        tempfloat = tempfloat - (float) digit; 
    }
    if (extra > 0 and not rightjustify) {
        for (int i = 0; i< extra; i++) {
            outstr[c++] = ' ';
        }
    }


    outstr[c++] = '\0';
    return outstr;
    
}



void setup() {
  while (!Serial);
    
   // Auto restart

   RestartCount = 0;
   
  Serial.begin(115200);
  Serial.println(F("CMAPS GPS sketch loaded"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while(1);
  }
  Serial.println(F("FONA is OK"));
  
  while (true) {
    uint8_t stat = fona.getNetworkStatus();
    if ((stat == 1) || (stat == 5)){
      Serial.print(F("Network associated!: "));
      Serial.println(stat);
      break;
    }
    delay(500);
      Serial.print(F("waiting for network: "));
      Serial.println(stat);
  }
  
  Serial.println(F("Setting APN"));
  fona.setGPRSNetworkSettings(F("internet")); //check your network provider
  fona.enableGPS(true);
  delay(5000); // change to 2000
  Serial.println(F("Setting GPRS status"));
  fona.enableGPRS(true);
  delay(5000);  //change to 10000

  ////////////  LCD I2C DISPLAY /////////////////////////
  lcd.begin(16,2); 
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home (); // go home
  
}

void loop() {
  // put your main code here, to run repeatedly:
  float lat, lon, speed_kph, bearing, altitude;
  boolean didGetLock = fona.getGPS(&lat, &lon, &speed_kph, &bearing, &altitude);

  if (didGetLock)
  {
    Serial.println(lat, 5); // the 4 gives us lat to four decimal places
    Serial.println(lon, 5);  // the 4 gives us lon to four decimal places
    uint16_t len;
    uint16_t statuscode;

    ////////////  LCD I2C DISPLAY /////////////////////////
    
    lcd.begin(16,2); 
    lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
    lcd.setBacklight(HIGH);
    
    //String lcdtopstring = "Lat: ";
    //lcdtopstring += lat;
    //String lcdbotstring = "Long: ";
    //lcdbotstring += lon; 
    //lcd.print(lcdtopstring); 
    lcd.print("Lat:"); lcd.print(lat, 3); // displays lat to 5 decimal places
    lcd.setCursor(0, 1); // Set cursor to bottom row
    lcd.print("Lon:"); lcd.print(lon, 3); // displays lon to 5 decimal places
    //lcd.print(lcdbotstring);
    lcd.setCursor(0, 0); // Set cursor to top row again
    //Next set cursor to second line display time

    
    //IMPORTANT "uuid=" defines the unique ID. LABEL THE ARDUINO UNIT with the number after upload.

// String url = "http://cmaps.knox.nsw.edu.au/location/push?uuid=1&latitude=-99&longitude=7777";
    String url = F("http://cmaps.knox.nsw.edu.au/location/push?");
    url += F("uuid=");
    url += String(DeviceUUID);
    url += F("&latitude=");

    char buff[25];
    floatToString(buff, lat , 8, 5);
    url += buff;
    url += F("&longitude=");
    floatToString(buff, lon , 8, 5);
    url += buff;

//    boolean httpGetStartRes = fona.HTTP_GET_start(buff, &statuscode, &len);
boolean httpGetStartRes = fona.HTTP_GET_start_From_String(&url, &statuscode, &len); //slightly changed to fix http issue MValent 17/3/2017
    fona.HTTP_GET_end();

    if (!httpGetStartRes)
    {
      RestartCount +=1;
      Serial.println("Restart Count: ");
      Serial.println( RestartCount);

      ////////////  LCD I2C DISPLAY /////////////////////////

      lcd.begin(16,2); 
      lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
      lcd.setBacklight(HIGH);
      lcd.home (); // go home
    
     lcd.print("Lat:"); lcd.print(lat, 5); // displays lat to 5 decimal places
    lcd.setCursor(0, 1); // Set cursor to bottom row
    lcd.print("Lon:"); lcd.print(lon, 5); // displays lon to 5 decimal places
    lcd.print("  F"); // F = Failed HTTP 
    lcd.setCursor(0, 0); // Set cursor to top row again
    //Next set cursor to second line display time
      
      if (RestartCount == 4) {
        RestartCount = 0;
        Serial.println("HTTP failure response, rebooting in 10s");
        delay(10000);
        asm volatile ("  jmp 0");
      }
   }
   else{
    RestartCount = 0;
   }

    // 5 minutes delay
    unsigned long start = millis();
    Serial.println("delay");    
    while (millis() < ((start+300000))) {
      delay(60000);
     }
    
    Serial.println("finished delay");    
  }
  else {
    Serial.println("No lock");

      ////////////  LCD I2C DISPLAY /////////////////////////

      lcd.begin(16,2); 
      lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
      lcd.setBacklight(HIGH);
      lcd.home (); // go home
    
      lcd.print("No lock."); 
    
  }
}
