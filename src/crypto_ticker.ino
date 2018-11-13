///A cryptocurrency ticker tape using ESP32 DEVKITV1, the P3 RGB Led matrix
///Uses:
///---> https://chasing-coins.com as the api endpoint for grabbing data 
///---> Arduino JSON parser library that for parsing api response 
///---> JSON Parser library:https://arduinojson.org/ 
#include <ArduinoJson.h>
#include <PxMatrix.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#define USE_SERIAL Serial

WiFiMulti wifiMulti;

// ESP32 Pins for LED MATRIX
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15  // NOT USED for 1/16 scan
#define P_OE 2
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

uint8_t display_draw_time=0;

PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);

// Red for negative change, Green for positive change
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);

void setup() {
 //WIFI setup
  USE_SERIAL.begin(115200);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for(uint8_t t = 4; t > 0; t--) {
      USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
      USE_SERIAL.flush();
      delay(1000);
   }
   
   wifiMulti.addAP("myRouter", "myRouterPassword");

}

void loop() {

  //Array of crypto symbols. Add as many as you like.
  const String crypto[] = {"LTC", "ETH", "BTC"};
  //Loop through array of crypto symbols
  for(int i = 0; i < 3; i++){
    printCrypto(crypto[i]);
  }

}
//Gets raw JSON from api endpoint. Pass it a crypto symbol
String getCoinData(String crypto){
        if((wifiMulti.run() == WL_CONNECTED)) {

          HTTPClient http;
          String endpoint = "https://chasing-coins.com/api/v1/std/coin/"+ crypto;
          USE_SERIAL.print("[HTTP] begin...\n");
          // Api endpoint for crypto data
          http.begin(endpoint);
  
          USE_SERIAL.print("[HTTP] GET...\n");
          // start connection and send HTTP header
          int httpCode = http.GET();
  
          // httpCode will be negative on error
          if(httpCode > 0) {
              // HTTP header has been send and Server response header has been handled
              USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
  
              // file found at server
              if(httpCode == HTTP_CODE_OK) {
                  String payload = http.getString();
                  USE_SERIAL.println(payload);
                  return payload;
              }
          } else {
              USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          }
  
          http.end();
      }
 }

void displayMessage(int msg_end, String message,float change){
  //display initalization done inside of this function, doing it inside
  // of void setup() causes issues related to multiwifi library, cause ESP 32 to 
  //contiunously reset and dump its core. If display_update_enable
  // is called before and multiwif functions, that when ESP 32 dumps core and resets.    
  display.begin(16); // 1/16 scan
  display.setFastUpdate(true);
  display_update_enable(true);
  display.setTextWrap(false);

  //if hourly change is positive print in green
  if(change > 0){
    display.setTextColor(myGREEN);
  }else{
    //otherwise show red 
    display.setTextColor(myRED);
  }
  //Scroll from right to left
  for(int i = PxMATRIX_MAX_WIDTH; i > msg_end; i--){
    display.clearDisplay();
    display.setCursor(i,7);
    display.println(message);
    delay(50);  
  }
}
//Truncates price to two decimal places, too lazy to round as currency prices
//fluctuate like mad anyway 
String truncatePrice(String price){
  int decimal = price.indexOf('.');
  price.remove(decimal + 3);
  return price; 

}
void printCrypto(String crypto){
  //JSON parsing
  const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 90;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  const String json = getCoinData(crypto);
  JsonObject& root = jsonBuffer.parseObject(json);
  
  //get price and percent change and convert change to float
  String change_str = root["change"]["hour"];
  float change = change_str.toFloat();
  String price = root["price"];

  //Message scrolls right to left, so need to find endpoint for message.
  //Matrix origin at top left corner, so end_point needs to be negative 
  String message = crypto + " = $" + truncatePrice(price);
  int msg_end = -6*message.length();//Characters  are 6 pixels wide
 
  displayMessage(msg_end, message, change); 
  
  delay(1500);
}
// ???? Something something interrupts, something something 
//rebel base. PxMatrix library uses timers to do its voodoo. 
void IRAM_ATTR display_updater(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}

void display_update_enable(bool is_enable)
{
if (is_enable)
  {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);
  }
else
  {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
}




