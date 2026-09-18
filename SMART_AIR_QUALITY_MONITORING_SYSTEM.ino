#define BLYNK_TEMPLATE_ID "TMPL34PpShGDc"
#define BLYNK_TEMPLATE_NAME "AIR QUALITY INDEX"
#define BLYNK_AUTH_TOKEN "RuYQJ2ki0lIfR_YzN3ILLQP2dzzcpsoW"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <DHT.h>

// WiFi Credentials
char ssid[] = "AndroidAP_9188"; 
char pass[] = "12345678";

// OLED SPI - Hardware SPI Pins: SCK=18, MOSI=23
#define OLED_CS 5
#define OLED_DC 16
#define OLED_RST 17

U8G2_SSD1327_MIDAS_128X128_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_DC, OLED_RST);

// Sensors
#define MQ135_PIN 34
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Outputs
#define RED_LED 25
#define YELLOW_LED 26
#define GREEN_LED 27
#define BUZZER 19 // Pin 32 is used to avoid SPI Hardware conflicts

#define BUZZER_CHN 0
#define BUZZER_RES 8

BlynkTimer timer;
float ppm, temp, hum;
int aqi;
String condition;

void buzzerBeep(bool state){
  if(state) ledcWriteTone(BUZZER_CHN, 2000);
  else ledcWriteTone(BUZZER_CHN, 0);
}

void readAndDisplay(){
  // 1. Read Sensors
  int raw = analogRead(MQ135_PIN);
  ppm = raw * 0.12;
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if(!isnan(t)) temp = t;
  if(!isnan(h)) hum = h;

  // 2. AQI Calculation
  int gasIndex = map(ppm, 0, 500, 0, 100);
  int tempIndex = map(temp, 20, 40, 0, 100);
  int humIndex = map(hum, 20, 80, 0, 100);
  aqi = (0.4 * gasIndex) + (0.3 * tempIndex) + (0.3 * humIndex);

  // 3. Logic for LEDs and Buzzer
  if(aqi <= 50){
    condition="GOOD";
    digitalWrite(GREEN_LED, HIGH); digitalWrite(YELLOW_LED, LOW); digitalWrite(RED_LED, LOW);
    buzzerBeep(false);
  }
  else if(aqi <= 100){
    condition="MODERATE";
    digitalWrite(GREEN_LED, LOW); digitalWrite(YELLOW_LED, HIGH); digitalWrite(RED_LED, LOW);
    buzzerBeep(false);
  }
  else{
    condition="POOR";
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    buzzerBeep(true);
  }

  // 4. OLED DISPLAY WITH UPDATED TITLE
  u8g2.clearBuffer();
  
  // Title Header
  u8g2.setFont(u8g2_font_7x14B_tf);
  u8g2.drawStr(0, 14, "SMART AIR QUALITY"); // Title updated here
  u8g2.drawHLine(0, 18, 128); 

  // Labels and Values
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setCursor(0, 38); u8g2.print("PPM: "); u8g2.print((int)ppm);
  u8g2.setCursor(0, 53); u8g2.print("TEMP: "); u8g2.print(temp, 1); u8g2.print(" C");
  u8g2.setCursor(0, 68); u8g2.print("HUM: "); u8g2.print(hum, 1); u8g2.print(" %");
  u8g2.setCursor(0, 83); u8g2.print("AQI: "); u8g2.print(aqi);
  u8g2.setCursor(0, 105); u8g2.print("COND: "); u8g2.print(condition);

  u8g2.sendBuffer();

  // 5. Send to Blynk
  if(Blynk.connected()){
    Blynk.virtualWrite(V0, aqi);
    Blynk.virtualWrite(V1, temp);
    Blynk.virtualWrite(V2, hum);
    Blynk.virtualWrite(V3, ppm);
    Blynk.virtualWrite(V4, condition);
  }
}

void setup(){
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  ledcSetup(BUZZER_CHN, 2000, BUZZER_RES);
  ledcAttachPin(BUZZER, BUZZER_CHN);

  dht.begin();

  // OLED Reset and Init
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); delay(50); digitalWrite(OLED_RST, HIGH);
  u8g2.begin();

  // Non-blocking WiFi/Blynk
  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);

  timer.setInterval(2000L, readAndDisplay);
}

void loop(){
  Blynk.run();
  timer.run();
}
