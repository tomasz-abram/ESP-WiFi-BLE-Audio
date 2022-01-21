#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <LiquidCrystal.h> // ekran do testów 

const char* ssid = "Kwarantanna#05050%12";
const char* password = "Cytryna77";

//MCP42010 deklaracje

const int csPin           = 10;       // MCP42100 chip select pin
const int  maxPositions    = 256;     // wiper can move from 0 to 255 = 256 positions
const long rAB             = 10000;  // 10k pot resistance between terminals A and B
const byte rWiper          = 39;     // 125 ohms pot wiper resistance
const byte pot0            = 0x11;    // pot0 addr
const byte pot1            = 0x12;    // pot1 addr
const byte potBoth         = 0x13;    // pot0 and pot1 simultaneous addr
const byte pot0Shutdown    = 0x21;    // pot0 shutdown
const byte pot1Shutdown    = 0x22;    // pot1 shutdown
const byte potBothShutdown = 0x23;    // pot0 and pot1 simultaneous shutdown

//Encoder

#define outputA 19 // dla encoder 
#define outputB 18 // dla encoder 

//LCD
LiquidCrystal lcd(17, 16, 4, 0, 2, 15);
int counter = 10;
int aState;
int aLastState;



void setup() {
  Serial.begin(115200);

//---------------------------------v START WIFI CONNECT AND UPDATE SECTION
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("AudioESP32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  
   //--------- potentiometr section
  digitalWrite(csPin, HIGH);        // chip select default to de-selected
  pinMode(csPin, OUTPUT);           // configure chip select as output
  SPI.begin();

  //---------------------------

  pinMode (outputA, INPUT); // A Encoder
  pinMode (outputB, INPUT); //B Encoder
  aLastState = digitalRead(outputA);  // Reads the initial state of the outputA
  lcd.begin(16, 2); // rozdzielczość ekranu

  //-----------------------------
  /*
    static const i2s_config_t i2s_config = {
          .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
          .sample_rate = 44100, // corrected by info from bluetooth
          .bits_per_sample = (i2s_bits_per_sample_t) 16,
          .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
          .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
          .intr_alloc_flags = 0, // default interrupt priority
          .dma_buf_count = 8,
          .dma_buf_len = 64,
          .use_apll = false
      };

      a2dp_sink.set_i2s_config(i2s_config);
      a2dp_sink.start("MyMusic");

  */




}

void loop() {


 aState = digitalRead(outputA); // Reads the "current" state of the outputA

  if (aState != aLastState) {    // If the previous and the current state of the outputA are different, that means a Pulse has occured
    if (digitalRead(outputB) != aState) { // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
      counter = counter + 5;
    } else {
      counter = counter - 5;
    }
    counter = constrain(counter, 0, 255);
    setPotWiper(pot1, counter);
    setPotWiper(pot0, counter);

    Serial.print("Position: ");
    Serial.println(counter);
    lcd.clear();
    lcd.print("Opor Ohm!");
    lcd.setCursor(0, 2);
    lcd.print(counter);
    lcd.print("=");
    lcd.print(counter * 39);
    aLastState = aState; // Updates the previous state of the outputA with the current state


  }

}

void setPotWiper(int addr, int pos) {

  pos = constrain(pos, 0, 255);            // limit wiper setting to range of 0 to 255

  digitalWrite(csPin, LOW);                // select chip
  SPI.transfer(addr);                      // configure target pot with wiper position
  SPI.transfer(pos);
  SPI.transfer(addr);                      // configure target pot with wiper position
  SPI.transfer(pos);
  SPI.transfer(addr);                      // configure target pot with wiper position
  SPI.transfer(pos);
  digitalWrite(csPin, HIGH);               // de-select chip

  // print pot resistance between wiper and B terminal
  long resistanceWB = ((rAB * pos) / maxPositions ) + rWiper;
  Serial.print(pos);
  Serial.print(resistanceWB);
  Serial.println(" ohms");

}