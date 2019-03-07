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

////////////  OLED I2C DISPLAY /////////////////////////
#include <Arduino.h>
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

// IMPORTANT "DeviceUUID" defines the unique ID. LABEL THE ARDUINO UNIT with the number after upload.
const int DeviceUUID = 91;

Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST); 
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);   
  
// floatToString.h    
//    
// Tim Hirzel   
// tim@growdown.com   
// March 2008   
// float to string    
//    
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
  pinMode(8,OUTPUT);
  digitalWrite(8, HIGH);

  u8x8.begin();
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
  
  u8x8.clear();
  u8x8.println(F("Starting..."));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    u8x8.clear();
    u8x8.println(F("No FONA"));
    while(1);
  }
  
  while (true) {
    uint8_t stat = fona.getNetworkStatus();
    if ((stat == 1) || (stat == 5)){
      break;
    }
  }
  fona.setGPRSNetworkSettings(F("internet")); //check your network provider
  fona.enableGPS(true);
  fona.enableGPRS(true);
  
}

void loop() {
  float lat, lon, speed_kph, bearing, altitude;
  boolean didGetLock = fona.getGPS(&lat, &lon, &speed_kph, &bearing, &altitude);

  if (!didGetLock) {
    u8x8.clear();
    u8x8.println(F("No Lock"));
    }
  else {
    uint16_t len;
    uint16_t statuscode;

    //Template url = "http://cmaps.knox.nsw.edu.au/location/push?uuid=1&latitude=-99&longitude=7777";
    String url = F("http://cmaps.knox.nsw.edu.au/location/push?");
    url += F("uuid=");
    url += String(DeviceUUID);
    url += F("&latitude=");

    u8x8.clear();
    char buff[10];
    String printString = F("Lat:");
    floatToString(buff, lat , 5, 5);
    url += buff;
    printString += buff;
    u8x8.println(printString);
    url += F("&longitude=");
    printString = F("Long:");
    floatToString(buff, lon , 5, 5);
    url += buff;
    printString += buff;
    u8x8.println(printString);

    boolean httpGetStartRes = fona.HTTP_GET_start_From_String(&url, &statuscode, &len);
    fona.HTTP_GET_end();

    if (httpGetStartRes)
    {
    u8x8.println(F("Sent!"));

    // 5 minutes delay
    unsigned long start = millis();  
    while (millis() < ((start+300000))) {
      delay(60000);
    }  
   }
  }
}
