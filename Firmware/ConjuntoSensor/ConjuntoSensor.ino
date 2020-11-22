#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

// Nome e senha da rede Station
char ssid_stt[] = "SUA-REDE-LOCAL"; 
char pw_stt[] = "SENHA-REDE-LOCAL"; 

// Nome e senha da rede AP
char ssid_ap[] = "REDE-CRIADA"; 
char password_ap[] = "SENHA-REDE-CRIADA-123"; 

const char* host = "192.168.4.1";
const int port = 80;

bool PIRstate ; 
bool lastPIRstate = HIGH;
int PIR = 2;

bool flag_fota = 0;

#define ID_SENSOR   "SENSOR1/r"

// Numero de bytes que quer acessar
#define EEPROM_SIZE 1

void sendTCP(void);
void InitOTA(void);

void setup() {
  Serial.begin(115200);
  
  EEPROM.begin(EEPROM_SIZE);

  if(EEPROM.read(0) == 100){
    Serial.println("\nEntrou no FOTA\n");
    EEPROM.write(0, 0);
    EEPROM.commit();
    InitOTA();
    flag_fota = 1;
  }
  else{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_ap, password_ap);
    Serial.println("Conectando no AP");
    while(WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print("."); 
     }
    Serial.println("CONECTADO NO AP\n");
  
    pinMode(PIR, INPUT);
    delay(30000);  // Aguardar 30s para o sensor funcionar corretamente
  }
}

void loop(){

  if(flag_fota){
      ArduinoOTA.handle();
  }
  else{
    PIRstate = digitalRead(PIR);  //HIGH quando detectado presença, else LOW
    Serial.println(PIRstate);
    if (PIRstate == HIGH) // Check se identificou presença
    {
      Serial.print("Sensor identificado\n");
      delay(1000);
      
      if (WiFi.status() == WL_CONNECTED)  //Check conexão WiFi
      {
        sendTCP();
      }
      lastPIRstate = PIRstate;
    }
  }
}

void sendTCP(void){
  WiFiClient client;
  uint8_t att_cnt=0;
  uint8_t status_error = false;
  String message;
  
  Serial.println("Conectando no Server");
  while(!client.connect(host, port)){ // Tenta conectar no concentrador
    Serial.println("...Conexão falhada!");
    delay(100); 
    att_cnt++;
    if(att_cnt > 30){
      status_error = true;
      break;
    }
  }
  if(!status_error){ // Se foi conectado, sera enviado a mensagem
    client.print(ID_SENSOR);
    Serial.println("mensagem TCP enviada!\n");
    while (client.connected() || client.available())
    {
      if (client.available()) // Analisa se tem alguma resposta
      {
        message = client.readStringUntil('\r');
        Serial.println(message);
      }
    }
    
    if(message == "ATUALIZACAO/r"){ // Se a resposta for ATUALIZACAO, sera resetado programando a posição 0 da EEPROM e resetado o sistema
      client.stop();
      EEPROM.write(0, 100);
      EEPROM.commit();
      ESP.restart();
    }
    client.stop();
    delay(2000); 
  }
  else{
    Serial.println("Reset\n");
    ESP.restart();
  }
}

void InitOTA(void){
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_h, password_h);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print("."); 
   }
  Serial.println("conectado ao home");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

