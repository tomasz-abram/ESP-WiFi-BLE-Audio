#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <LiquidCrystal.h> // ekran do testów
#include <ESP32Encoder.h>
#include "BluetoothA2DPSink.h"

//MCP42010 deklaracje
const int csPin = 5;              // MCP42100 chip select 
const int maxPositions = 256;      // wiper can move from 0 to 255 = 256 positions
const long rAB = 10000;            // 10k pot resistance between terminals A and B
const byte rWiper = 39;            // 125 ohms pot wiper resistance
const byte pot0 = 0x11;            // pot0 addr
const byte pot1 = 0x12;            // pot1 addr
const byte potBoth = 0x13;         // pot0 and pot1 simultaneous addr
const byte pot0Shutdown = 0x21;    // pot0 shutdown
const byte pot1Shutdown = 0x22;    // pot1 shutdown
const byte potBothShutdown = 0x23; // pot0 and pot1 simultaneous shutdown
void setPotWiper(int addr, int pos);
void lcdScreen(int eCounter);

//Encoder
int iCounter(int iState);
int lastCounter;
int eCounter = 50;
int iState;
int iLastState;
ESP32Encoder encoder;

//LCD
LiquidCrystal lcd(17, 16, 4, 0, 2, 15);

//BLE
BluetoothA2DPSink a2dp_sink;
const int mutePin = 13;

// Wi-Fi update
const char *ssid = "Kwarantanna#05050%12";
const char *password = "Cytryna77";

void setup()
{
  Serial.begin(115200);
 
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
      .onStart([]()
               {
                 String type;
                 if (ArduinoOTA.getCommand() == U_FLASH)
                   type = "sketch";
                 else // U_SPIFFS
                   type = "filesystem";

                 // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 Serial.println("Start updating " + type);
               })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
                 Serial.printf("Error[%u]: ", error);
                 if (error == OTA_AUTH_ERROR)
                   Serial.println("Auth Failed");
                 else if (error == OTA_BEGIN_ERROR)
                   Serial.println("Begin Failed");
                 else if (error == OTA_CONNECT_ERROR)
                   Serial.println("Connect Failed");
                 else if (error == OTA_RECEIVE_ERROR)
                   Serial.println("Receive Failed");
                 else if (error == OTA_END_ERROR)
                   Serial.println("End Failed");
               });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //--------- potentiometr section--------------------------------------------------------------
  digitalWrite(csPin, HIGH); // chip select default to de-selected
  pinMode(csPin, OUTPUT);    // configure chip select as output
  SPI.begin();

  //---------------Encoder----------------------------------------------------------------------
  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachSingleEdge(21, 19);
  Serial.println("Encoder Start = " + (int32_t)encoder.getCount());
  encoder.setFilter(uint16_t(1023));

  //---------------LCD--------------------------------------------------------------------------
  lcd.begin(16, 2); // rozdzielczość ekranu

  //---------------Music------------------------------------------------------------------------
pinMode(mutePin, LOW);
i2s_pin_config_t my_pin_config = {
        .bck_io_num = 12,
        .ws_io_num = 27,
        .data_out_num = 14,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
  static i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = 44100, // updated automatically by A2DP
      .bits_per_sample = (i2s_bits_per_sample_t)32,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = 0, // default interrupt priority
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = true,
      .tx_desc_auto_clear = true // avoiding noise in case of data unavailability
  };
  a2dp_sink.set_i2s_config(i2s_config);
  a2dp_sink.set_pin_config(my_pin_config);
  a2dp_sink.start("M1000");

  /*
void setup() {
    i2s_pin_config_t my_pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 22,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(my_pin_config);
    a2dp_sink.start("MyMusic");
*/
}

void loop()
{
  ArduinoOTA.handle();
  iCounter((int32_t)encoder.getCount());

  if (eCounter != lastCounter)
  {
    lcdScreen(eCounter);
    setPotWiper(pot1, eCounter);
    setPotWiper(pot0, eCounter);
    lastCounter = eCounter;
  }
}

int iCounter(int iState)
{

  if (iState != iLastState)
  { // If the previous and the current state of the outputA are different, that means a Pulse has occured
    if (iState > iLastState)
    { // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
      eCounter = eCounter + 5;
    }
    else
    {
      eCounter = eCounter - 5;
    }
    iLastState = iState;
    eCounter = constrain(eCounter, 0, 255);
  }
  return eCounter;
}

void lcdScreen(int lcdCounter)
{
  lcd.clear();
  lcd.print("Opor Ohm!");
  lcd.setCursor(0, 2);
  lcd.print(lcdCounter);
  lcd.print("=");
  lcd.print(lcdCounter * 39);
}

void setPotWiper(int addr, int pos)
{

  pos = constrain(pos, 0, 255); // limit wiper setting to range of 0 to 255

  digitalWrite(csPin, LOW); // select chip
  SPI.transfer(addr);       // configure target pot with wiper position
  SPI.transfer(pos);
  SPI.transfer(addr); // configure target pot with wiper position
  SPI.transfer(pos);
  SPI.transfer(addr); // configure target pot with wiper position
  SPI.transfer(pos);
  digitalWrite(csPin, HIGH); // de-select chip

  // print pot resistance between wiper and B terminal
  long resistanceWB = ((rAB * pos) / maxPositions) + rWiper;
  Serial.print(pos);
  Serial.print(resistanceWB);
  Serial.println(" ohms");
}