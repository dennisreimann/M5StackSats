#include "lnbits.c"
#include <M5Stack.h> 
#include <string.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <math.h>

#define KEYBOARD_I2C_ADDR     0X08
#define KEYBOARD_INT          5

////////////////////////DETAILS TO CHANGE///////////////////////////////////////

//WIFI DETAILS
char wifiSSID[] = "PLUSNET-XPUGF_EXT"; //ENTER YOUR WIFI SSID (Case sensitive)
char wifiPASS[] = "PaSSwOrD123"; //ENTER YOUR WIFI PASSWORD

//LNBITS DETAILS
const char* lnbitshost = "lnbits.com"; //ENTER YOUR LNBITS HOST
String invoicekey = "63d93848c0fa44e3a0bc341cbdfb9735"; //ENTER YOUR LNBITS INVOICE KEY

//OTHER DETAILS
String memo = "Memo "; //(OPTIONAL) CHANGE THE MEMO SUFFIX
String on_currency = "BTCGBP"; //(OPTIONAL) CHANGE THE SHITCOIN FIAT

////////////////////////////////////////////////////////////////////////////////

int httpsPort = 443;
String pubkey;
String totcapacity;
const char* payment_request;
bool certcheck = false;

String choice;
String payhash;
String on_sub_currency = on_currency.substring(3);


  String key_val;
  String cntr = "0";
  String inputs;
  int keysdec;
  int keyssdec;
  float temp;  
  String fiat;
  float satoshis;
  String nosats;
  float conversion;
  String postid;
  String data_id;
  String data_lightning_invoice_payreq = "";
  String data_status;
  bool settle = false;
  String payreq;
  String hash;

void page_input()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.println("Amount then C");
  M5.Lcd.println("");
  M5.Lcd.println(on_currency.substring(3) + ": ");
  M5.Lcd.println("");
  M5.Lcd.println("SATS: ");
  M5.Lcd.println("");
  M5.Lcd.println("");
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(50, 200);
  M5.Lcd.println("TO RESET PRESS A");
}

void page_processing()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(40, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PROCESSING");
 
}

void get_keypad(){

   if(digitalRead(KEYBOARD_INT) == LOW) {
    Wire.requestFrom(KEYBOARD_I2C_ADDR, 1);  // request 1 byte from keyboard
    while (Wire.available()) { 
       uint8_t key = Wire.read();                  // receive a byte as character
       key_val = key;

       if(key != 0) {
        if(key >= 0x20 && key < 0x7F) { // ASCII String
          if (isdigit((char)key)){
          key_val = ((char)key);
          }
          else {
          key_val = "";
        } 
        }
      }
    }
  }
}

void setup() {
  M5.begin();
  M5.Lcd.drawBitmap(0, 0, 320, 240, (uint8_t *)lnbits_map);
  Wire.begin();
  Serial.begin(115200);
  //connect to local wifi            
  WiFi.begin(wifiSSID, wifiPASS);   
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if(i >= 5){
     M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(55, 80);
     M5.Lcd.setTextSize(2);
     M5.Lcd.setTextColor(TFT_RED);
     M5.Lcd.println("WIFI NOT CONNECTED");
    }
    delay(1000);
    i++;
  }
  on_rates();
    
  pinMode(KEYBOARD_INT, INPUT_PULLUP);
  
  
 
}

void loop() {
 
 page_input();

  cntr = "1";
  while (cntr == "1"){
   M5.update();
   get_keypad(); 

    if (M5.BtnC.wasReleased()) {

     page_processing();
     
     reqinvoice(nosats);

     page_qrdisplay(payreq);

  ///////
  checkpaid();
     
     key_val = "";
     inputs = "";
    }


    
     else if (M5.BtnB.wasReleased()) {

      page_processing();
    
      nosats = "0";
      reqinvoice(nosats);
      page_qrdisplay(payreq);
      checkpaid();
      key_val = "";
     inputs = "";
       
    }

    
     else if (M5.BtnA.wasReleased()) {

     M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(0, 0);
     M5.Lcd.setTextColor(TFT_WHITE);
      page_input();
      key_val = "";
      inputs = "";  
      nosats = "";
    }
    
    
      Serial.print(key_val);
      inputs += key_val;
      
      temp = inputs.toInt();
      
      temp = temp / 100;

      fiat = temp;
      
      satoshis = temp/conversion;

      int intsats = (int) round(satoshis*100000000.0);

      nosats = String(intsats);
        M5.Lcd.setTextSize(3);
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5.Lcd.setCursor(70, 88);
        M5.Lcd.println(fiat);
        M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Lcd.setCursor(87, 136);
        M5.Lcd.println(nosats);


      delay(100);
      key_val = "";
        
    
    
  }
}



//OPENNODE REQUESTS

void on_rates(){
  WiFiClientSecure client;
  if (!client.connect("api.opennode.co", httpsPort)) {
    return;
  }

  String url = "/v1/rates";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.opennode.co\r\n" +
               "User-Agent: ESP32\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
    const size_t capacity = 169*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(168) + 3800;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, line);
    conversion = doc["data"][on_currency][on_currency.substring(3)]; 

}
// PAYWALL REQUESTS

void reqinvoice(String value){

 WiFiClientSecure client;
  
  if (!client.connect(lnbitshost, httpsPort)) {
    return;
  }
  
  String topost = "{  \"out\" : false, \"amount\" : " + nosats +", \"memo\" :\""+ memo + String(random(1,1000)) + "\"}";
  String url = "/api/v1/payments";
  client.print(String("POST ") + url +" HTTP/1.1\r\n" +
                "Host: " + lnbitshost + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key:"+ invoicekey +"\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + topost.length() + "\r\n" +
                "\r\n" + 
                topost + "\n");



  while (client.connected()) {
    String line = client.readStringUntil('\n');
   Serial.println(line);
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readString();


  Serial.println(line);
  const size_t capacity = JSON_OBJECT_SIZE(2) + 800;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, line);
  const char* pay_req = doc["payment_request"]; 
  const char* payment_hash = doc["checking_id"]; 
  payreq = pay_req;
  Serial.println(payreq);
  payhash = payment_hash;
  Serial.println(payhash);
}


void checkpaid(){
     delay(2000);
     int counta = 0;
     int tempi = 0;
   
     while (tempi == 0){

     checkpayment();
     if (settle == false){
       
        counta ++;
        Serial.print(counta);
        if (counta == 100){
         tempi = 1;
        }
     }
      
       else{
        tempi = 1;

     M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(60, 80);
     M5.Lcd.setTextSize(4);
     M5.Lcd.setTextColor(TFT_GREEN);
     M5.Lcd.println("COMPLETE");
     settle = false;
     delay(1000);
   
     cntr = "2";
 
      }
      
     int bee = 0;
     while ((bee < 120) && (tempi==0)){

      M5.update();

     if (M5.BtnA.wasReleased()) {

        tempi = -1;
     
     M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(50, 80);
     M5.Lcd.setTextSize(4);
     M5.Lcd.setTextColor(TFT_RED);
     M5.Lcd.println("CANCELLED");

     delay(1000);
   
     cntr = "2";
     
      }
      
      delay(10);
      bee++;
     key_val = "";
     inputs = "";
     
     }
     
      
     }
}


void checkpayment(){
  WiFiClientSecure client;
  
  if (!client.connect(lnbitshost, httpsPort)) {
    return;
  }
  String url = "/api/v1/payments/";
  client.print(String("GET ") + url + payhash +" HTTP/1.1\r\n" +
                "Host: " + lnbitshost + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key:"+ invoicekey +"\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");

   String line = client.readStringUntil('\n');

  Serial.println(line);
  const size_t capacity = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, line);
  settle = doc["paid"]; 
  Serial.println(settle);
}

void page_qrdisplay(String xxx)
{  

  M5.Lcd.fillScreen(BLACK); 
  M5.Lcd.qrcode(payreq,45,0,240,14);
  delay(100);

}
