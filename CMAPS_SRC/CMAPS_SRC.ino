#include <Adafruit_FONA.h>

#include "Adafruit_FONA.h"
#include <stdlib.h>

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

#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //Using the page buffer part of this library, to be less RAM intensive (full buffer + FONA was exceeding the memory on the Arduino)
                                                                             //Should RAM usage be a problem in the future, consider using the Uxgx part instead (loses some other graphical features, but even further reduced memory usage)

bool devMode = true; //Turns on debug messages to the OLED Display, consider keeping on all the time for feedback considerations?

using namespace std;

const int DeviceUUID = 502; // IMPORTANT "DeviceUUID" defines the unique ID. LABEL THE ARDUINO UNIT with the number after upload.

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

void OLEDPrint(String text) { //Use for printing one line of text on the OLED display. Does not consider line length at all and will just print what fits within one line
    const char* string1 = text.c_str(); //We must use character pointers since u8g2.drawStr() requires a character pointer in its parameters (not a string!), note that it will treat this pointer like a character array and iterate until it finds the null
    u8g2.clear();
      do {
        u8g2.drawStr(0,12,string1);
      } while ( u8g2.nextPage() );
  }

void debugPrint(String text) { //Call this when printing something of debug/devMode purpose to the OLED
      if(devMode) {
          OLEDPrint(text);  
      }
  }
  

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
  u8g2.begin();
  u8g2.clear();
  u8g2.setFont(u8g2_font_6x10_tr); //Many different fonts avaliable at https://github.com/olikraus/u8g2/wiki/fntlistall
  while (!Serial);
    
   // Auto restart

   RestartCount = 0;
   
  Serial.begin(115200);
  Serial.println(F("CMAPS GPS sketch loaded"));
  Serial.println(F("Initializing....(May take 3 seconds)"));
  debugPrint("Starting...");

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    debugPrint("No FONA Found");
    while(1);
  }
  Serial.println(F("FONA is OK"));
  debugPrint("FONA is OK");
  
  while (true) {
    uint8_t stat = fona.getNetworkStatus();
    if ((stat == 1) || (stat == 5)){
      Serial.print(F("Network associated!: "));
      Serial.println(stat);
      debugPrint("Network Associated");
      break;
    }
    delay(500);
      Serial.print(F("waiting for network: "));
      Serial.println(stat);
      debugPrint("Waiting for network");
  }
  
  Serial.println(F("Setting APN"));
  debugPrint("Setting APN");
  fona.setGPRSNetworkSettings(F("internet")); //check your network provider
  fona.enableGPS(true);
  delay(5000); // change to 2000
  Serial.println(F("Setting GPRS status"));
  debugPrint("Setting GPRS status");
  fona.enableGPRS(true);
  delay(5000);  //change to 10000
}

void loop() {
  float lat, lon, speed_kph, bearing, altitude;
  boolean didGetLock = fona.getGPS(&lat, &lon, &speed_kph, &bearing, &altitude);
  
  
  if (didGetLock)
  {
    Serial.println(lat, 5); // the 4 gives us lat to four decimal places
    Serial.println(lon, 5);  // the 4 gives us lon to four decimal places
    uint16_t len;
    uint16_t statuscode;

    u8g2.clear();
    do {
      char lati[10];
      dtostrf(lat, 4, 6, lati); //effectively the same as floatToString() defined above, but keeps it in character array form instead of a string
      u8g2.drawStr(0,12,"Lat:");
      u8g2.drawStr(0,25, lati);
      char longi[10]; //these are done immediately before the drawStr() because of strange memory glitches, and this was the only solution found
      dtostrf(lon, 4, 6, longi); //a tad messy and unconventional, but gets the job done. Especially since these values are only used for printing on the OLED and not actually sent over 3G
      u8g2.drawStr(0,38,"Long:"); //For dtostrf() documentation, search for "avr libc dtostrf", but there isn't very much
      u8g2.drawStr(0,51, longi);
    } while ( u8g2.nextPage() );
    delay(1000);
    //m++;
    //if ( m == 60 )
      //m = 0;

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
    char url_chars[url.length()+1];
    url.toCharArray(url_chars, url.length()+1);
    boolean httpGetStartRes = fona.HTTP_GET_start(url_chars, &statuscode, &len); //slightly changed to fix http issue MValent 17/3/2017
    fona.HTTP_GET_end();

    if (!httpGetStartRes)
    {
      RestartCount +=1;
      Serial.println("Restart Count: ");
      Serial.println( RestartCount);
      
      u8g2.clear();
      do {
        char lati[10];
        dtostrf(lat, 4, 6, lati);
        u8g2.drawStr(0,12,"Lat:");
        u8g2.drawStr(0,25, lati);
        char longi[10];
        dtostrf(lon, 4, 6, longi);
        u8g2.drawStr(0,38,"Long:");
        u8g2.drawStr(0,51, longi);
        u8g2.drawStr(0, 64, "Send To Server Fail"); //Some remnant from previous LCD code? Kept here incase needed
      } while ( u8g2.nextPage() );
      
      
      if (RestartCount == 4) {
        RestartCount = 0;
        Serial.println("HTTP failure response, rebooting in 10s");
        OLEDPrint("HTTP Failure, rebooting");
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
    OLEDPrint("No Lock");
  }
}

