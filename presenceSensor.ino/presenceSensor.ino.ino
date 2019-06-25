
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define DEBUG

#define PIR_SENSOR 5 // GPIO5
#define LED_PIN 13   //GPIO13

//informações da rede WIFI
const char* ssid = "WIFI_SSID";                             
const char* password =  "WIFI_PASS";                  

const char* mqttServer = "MQTT_SERVER_IP_OR_HOSTNAME";           
const int mqttPort = 1883;                                
const char* mqttTopicPresence = "/se/prese/930ASD";       //tópico para o sensor de presença
const char* mqttTopicLed = "/se/led/930ASD";              //tópico para o LED

int disableLed = 0;
int oldPresenceValue = 0;
 
WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
 
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_SENSOR, INPUT);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
    Serial.println("Conectando ao WiFi..");
    #endif
  }
  #ifdef DEBUG
  Serial.println("Conectado na rede WiFi");
  #endif
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.println("Conectando ao Broker MQTT...");
    #endif
 
    if (client.connect("ESP8266Client")) {
      #ifdef DEBUG
      Serial.println("Conectado ao broker");  
      #endif
 
    } else {
      #ifdef DEBUG 
      Serial.print("falha estado  ");
      Serial.print(client.state());
      #endif
      delay(2000); 
    }
  }

  //subscreve no tópico
  client.subscribe(mqttTopicPresence);
  client.subscribe(mqttTopicLed);
 
}
 
void callback(char* topic, byte* payload, unsigned int length) {

  //armazena msg recebida em uma sring
  payload[length] = '\0';
  String message = String((char*)payload);
  
  #ifdef DEBUG
  Serial.print("Mensagem chegou do tópico: ");
  Serial.println(topic);
  Serial.print("Mensagem:");
  Serial.print(message);
  Serial.println();
  Serial.println("-----------------------");
  #endif

  if (strcmp(topic, mqttTopicLed) == 0){
    if (message == "1"){      
      disableLed = 0;   // estará ativado  
    }else if (message == "0"){   
      digitalWrite(LED_PIN, LOW);
      disableLed = 1;   // estará desativado
    }
    
  } else if (strcmp(topic, mqttTopicPresence) == 0){
        
    if (message == "0"){      
      digitalWrite(LED_PIN, LOW); 
    }else if (message == "1" && disableLed == 0){   
      digitalWrite(LED_PIN, HIGH);  
    }
    
  }
}

//função pra reconectar ao servido MQTT
void reconect() {
  //Enquanto estiver desconectado
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.print("Tentando conectar ao servidor MQTT");
    #endif
     
    bool conectado = client.connect("ESP8266Client");

    if(conectado) {
      #ifdef DEBUG
      Serial.println("Conectado!");
      #endif
      //subscreve no tópico
      client.subscribe(mqttTopicPresence, 1); //nivel de qualidade: QoS 1
      client.subscribe(mqttTopicLed, 1);
    } else {
      #ifdef DEBUG
      Serial.println("Falha durante a conexão.Code: ");
      Serial.println( String(client.state()).c_str());
      Serial.println("Tentando novamente em 10 s");
      #endif
      //Aguarda 10 segundos 
      delay(10000);
    }
  }
}

void DetectPresence(){

  int readedValue = digitalRead(PIR_SENSOR);
  
  if (client.connected()) {
    if (readedValue != oldPresenceValue){
      oldPresenceValue = readedValue;
      if (readedValue == 1) {
        client.publish(mqttTopicPresence, "1"); //detectou 
      }else{
        client.publish(mqttTopicPresence, "0"); //não detectou
      }
    }
  }
}
 
void loop() {
  if (!client.connected()) {
    reconect();
  }
  DetectPresence();  
  client.loop();
}
