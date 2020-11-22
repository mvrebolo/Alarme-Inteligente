
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <stdio.h>
#include <string.h>

void RecebeMSGTelegram(void);
void RecebeMSGTCP(void);
void handleNewMessages(int numNewMessages);
void sendMessageTelegram(String message);

// Mensagens recebidas dos sensores
#define SENSOR_1        "SENSOR1/r"
#define SENSOR_2        "SENSOR1/r"
#define SENSOR_3        "SENSOR1/r"
#define SENSOR_4        "SENSOR1/r"

// Mensagem que será enviada para os sensores em caso de FOTA
#define FOTA            "ATUALIZACAO/r"

// Pino do Rele que aciona a sirene
#define PIN_RELE    1

// IDs dos usuários do Telegram
#define TELEGRAM_MARCUS     "xxxxxxxxx"
#define TELEGRAM_FULANO1    "xxxxxxxxx"
#define TELEGRAM_FULANO2    "xxxxxxxxx"
#define TELEGRAM_FULANO3    "xxxxxxxxx"
#define TELEGRAM_FULANO4    "xxxxxxxxx"

// Token para inicializar bot do Telegram (Pegar na criação do BotFather)
#define BOTtoken "1481349047:AAFg2CgiZTWJeSlgPujDxTmkH73uYJITYBE" 

//Quantidade de usuários que podem interagir com o bot
#define SENDER_ID_COUNT 5

// WiFi no modo AP
char ssid_ap[] = "REDE-CRIADA";           
char pw_ap[] = "SENHA-REDE-CRIADA-123"; 
          
WiFiServer server(80);	//Porta do server
WiFiClient client_ap;	//Client (sensor) que irá mandar mensagem via rede criada pelo Server
  
// WiFi no modo STATION
char ssid_stt[] = "SUA-REDE-LOCAL";     
char pw_stt[] = "SENHA-REDE-LOCAL"; 

// Fingerprint Telegram (https://www.grc.com/fingerprints.htm -> api.telegram.org)
const uint8_t fingerprint[20] = {0xF2, 0xAD, 0x29, 0x9C, 0x34, 0x48, 0xDD, 0x8D, 0xF4, 0xCF, 0x52, 0x32, 0xF6, 0x57, 0x33, 0x68, 0x2E, 0x81, 0xC1, 0x90};

//Ids dos usuários que podem interagir com o bot. 
//É possível verificar seu id pelo monitor serial ao enviar uma mensagem para o bot
String validSenderIds[SENDER_ID_COUNT] = {TELEGRAM_MARCUS, TELEGRAM_FULANO1, TELEGRAM_FULANO2, TELEGRAM_FULANO3, TELEGRAM_FULANO4};

WiFiClientSecure client; // Client (Telegram) que manda ou recebe mensagem via rede local
UniversalTelegramBot bot(BOTtoken, client); //Inicia bot com o token do bot criado no Telegram

int Bot_mtbs = 1000; //Tempo entre scans de mensagens Telegram
long Bot_lasttime;   //tempo anterior da ultima mensagem recebida

// Variaveis para controle
bool flagAlarm = 0;
bool alarmeDisparado = 0;
bool flag_fota1 = 0;
bool flag_fota2 = 0;
bool flag_fota3 = 0;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid_ap, pw_ap); //Nome e senha da rede criada
  delay(2000);

  WiFi.begin(ssid_stt, pw_stt); //Nome e senha da rede local a se conectar.

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("WiFi connected");
  delay(100);
  server.begin();
  client.setFingerprint(fingerprint); //setando fingerprint do Telegram

  pinMode(PIN_RELE, OUTPUT); //Declarando pino do relé
  digitalWrite(PIN_RELE, LOW); 
}

void loop() {
  RecebeMSGTelegram();
  RecebeMSGTCP();
}

void RecebeMSGTelegram(void){
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    Bot_lasttime = millis();
  }
}

void RecebeMSGTCP(void){
  
client_ap = server.available();

  if(client_ap){ // Tem algum cliente querendo conectar
    Serial.println("Client connected!\n");
    String message = client_ap.readStringUntil('\r'); // Lê mensagem recebida
    Serial.println(message);
        
    if(message.compareTo(SENSOR_1) == 0){
      if(flag_fota1){ // Solicitação de FOTA
        flag_fota1 = FALSE;
        client_ap.print(FOTA);
        bot.sendMessage(TELEGRAM_MARCUS, "FOTA ID 1 habilitado\n", "");
      }
      if(flagAlarm == TRUE){ //Se for verdadeiro, sirene irá tocar
        digitalWrite(PIN_RELE,HIGH);
        alarmeDisparado = TRUE;
      }
      sendMessageTelegram("Sensor 1 identificado\n");
    }

    if(message.compareTo(SENSOR_2) == 0){
      if(flag_fota2){ // Solicitação de FOTA
        client_ap.print(FOTA);
        flag_fota2 = FALSE;
        bot.sendMessage(TELEGRAM_MARCUS, "FOTA ID 2 habilitado\n", "");
      }
      if(flagAlarm == TRUE){ //Se for verdadeiro, sirene irá tocar
        digitalWrite(PIN_RELE,HIGH);
        alarmeDisparado = TRUE;
      }
      sendMessageTelegram("Sensor 2 identificado\n");
    }

    if(message.compareTo(SENSOR_3) == 0){
      if(flagAlarm == TRUE){ //Se for verdadeiro, sirene irá tocar
        digitalWrite(PIN_RELE,HIGH);
        alarmeDisparado = TRUE;
      }
      if(flag_fota3){ // Solicitação de FOTA
        client_ap.print(FOTA);
        flag_fota3 = FALSE;
        bot.sendMessage(TELEGRAM_MARCUS, "FOTA ID 3 habilitado\n", "");
      }
      sendMessageTelegram("Sensor 3 identificado\n");
    }
    client_ap.stop();
  }
}

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String senderId = String(bot.messages[i].from_id); //id do contato
    String from_name = bot.messages[i].from_name;
    
    Serial.println("senderId: " + senderId); //mostra no monitor serial o id de quem mandou a mensagem
    Serial.println("Name: " + from_name); //mostra no monitor serial o nome de quem mandou a mensagem
    boolean validSender = validateSender(senderId); //verifica se é o id de um remetente da lista de remetentes válidos
 
    if(!validSender) //se não for um remetente válido
    {
      bot.sendMessage(chat_id, "EAI " + from_name +  " VOCE NAO TEM PERMISSÃO HAHAHAHAHA", ""); //envia mensagem que não possui permissão e retorna sem fazer mais nada
      bot.sendMessage(TELEGRAM_MARCUS, "Marcus, " + from_name +  " com ID " + senderId + ", tentou mandar mensagem para o bot","");   //Manda pro meu telegram
      continue; //continua para a próxima iteração do for (vai para próxima mensgem, não executa o código abaixo)
    }
    
    String text = bot.messages[i].text;
    if (from_name == "") from_name = "Sem nome";

    if (text == "/ligaralarme") {
      flagAlarm = 1;
      
      digitalWrite(PIN_RELE,HIGH);
      delay(100);
      digitalWrite(PIN_RELE,LOW);
      delay(100);
      digitalWrite(PIN_RELE,HIGH);
      delay(100);
      digitalWrite(PIN_RELE,LOW);
      
      bot.sendMessage(chat_id, "Alarme ligado, " + from_name + "\n","");
    }

    if (text == "/desligaralarme") {
      flagAlarm = 0;
      alarmeDisparado = 0;
      
      digitalWrite(PIN_RELE,LOW);
      delay(100);
      digitalWrite(PIN_RELE,HIGH);
      delay(200);
      digitalWrite(PIN_RELE,LOW);
      
      bot.sendMessage(chat_id, "Alarme desligado, " + from_name + "\n","");
    }

    if (text == "/estado") {
      if(flagAlarm){
        bot.sendMessage(chat_id, "Alarme está ligado, " + from_name + "\n", "");
        if(alarmeDisparado){
           bot.sendMessage(chat_id, "Cuidado, " + from_name + ", o alarme está disparado\n", "");
        }
      } 
      else {
        bot.sendMessage(chat_id, "Alarme está desligado, " + from_name + "\n", "");
      }
    }
   
    if(text == "/ATUALIZACAO1"){
      flag_fota1 = true;  
      bot.sendMessage(chat_id, "Ok, " + from_name + "\n", "");
    }

    if(text == "/ATUALIZACAO2"){
      flag_fota2 = true;  
      bot.sendMessage(chat_id, "Ok, " + from_name + "\n", "");
    }

    if(text == "/ATUALIZACAO3"){
      flag_fota3 = true;  
      bot.sendMessage(chat_id, "Ok, " + from_name + "\n", "");
    }

    if((text == "/inicia") || (text == "/ajuda")){
      String welcome = "Fala ae, " + from_name + ".\n";
      welcome += "Como você está? De boa ai???\n";
      welcome += "Pode mandar os comandos:\n";
      welcome += "/ligaralarme: Ligar o alarme\n";
      welcome += "/desligaralarme: Desligar o alarme\n";
      welcome += "/estado: Verificar o estado do alarme\n\n";
      welcome += "VALEU!!\n";
      bot.sendMessage(chat_id, welcome, "");
    }
  }
}

boolean validateSender(String senderId)
{
  //Para cada id de usuário que pode interagir com este bot
  for(int i=0; i<SENDER_ID_COUNT; i++)
  {
    //Se o id do remetente faz parte do array, é retornado que é válido
    if(senderId == validSenderIds[i])
    {
      return true;
    }
  }
 
  //Se chegou aqui significa que verificou todos os ids e não encontrou no array
  return false;
}

void sendMessageTelegram(String message){
  bot.sendMessage(TELEGRAM_MARCUS, message,"");
  bot.sendMessage(TELEGRAM_FULANO1, message,"");
  bot.sendMessage(TELEGRAM_FULANO2, message,"");
  bot.sendMessage(TELEGRAM_FULANO3, message,"");
  bot.sendMessage(TELEGRAM_FULANO4, message,"");
}

