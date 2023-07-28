#include <SPI.h>
#include "RF24.h"
#include <ESP8266WiFi.h>
#include <espnow.h>

/* Definições */
#define NUMBER_OF_ROBOTS 2

/* Pinos para o rádio */
int CE = 12;
int CS = 13;

/* Objeto que gerencia o rádio */
RF24 radio(CE,CS);

/* Endereços */
uint64_t txAddresses[] = {0xABCDABCD71LL, 0x544d52687CLL, 0x644d52687CLL};

//Coloque na linha abaixo o Mac Address do NodeMCU receptor
uint8_t broadcastAddress[] = {0x60, 0x01, 0x94, 0x0A, 0x87, 0x72};

/* Estrutura para a mensagem a ser transmitida para o robô via rádio */
struct Velocidade{
  int16_t vl[3];
  int16_t vr[3];
};

struct VelocidadeRadio{
  int16_t vl;
  int16_t vr;
};

/* Estrutura para a mensagem a ser recebida do USB */
struct VelocidadeSerial {
  Velocidade data;
  int16_t checksum;
};

/* Declaração das funções */
void wifiSetup();
void sendWifi();
void receiveUSBdata();

/* Mensagem a ser transmitida */
Velocidade velocidades;

/* Contagem de erros de transmissão via USB detectados */
uint32_t erros = 0;
uint32_t lastOK = 0;
bool result;

//Callback quando os dados sao enviados
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
  if (sendStatus == 0)
    result = true;
  else
    result = false;
}

/* Loop de setup */
void setup(void) {
  Serial.begin(115200);
  while(!Serial);
  wifiSetup();
  pinMode(LED_BUILTIN, OUTPUT);
}

/* Loop que é executado continuamente */
void loop(){
    // Recebe velocidades via USB
    receiveUSBdata();

    // Envia via rádio
		static int32_t t = micros();
		if(micros()-t >= 500){
			t = micros();
      sendWifi();
		}

    // Acende o LED se recebeu mensagem do USB em menos de 5ms
    if(millis()-lastOK < 5){
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else{
      digitalWrite(LED_BUILTIN, LOW);
    }

}

/* Envia a mensagem pelo rádio */
void sendWifi(){
  
  // Envia a mensagem
  result = true;

  for(uint8_t i=0 ; i<NUMBER_OF_ROBOTS ; i++){
    VelocidadeRadio vel = {.vl = velocidades.vl[i], .vr = velocidades.vr[i]};
    
    //Envia a mensagem usando o ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &vel, sizeof(VelocidadeRadio));

  }
  if(result){
    lastOK = millis();
  }
}

/* Configura o wi-fi */
void wifiSetup(){
  //Coloca o dispositivo no modo Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Inicializa o ESP-NOW
  if (esp_now_init() != 0) {
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  //Registra o destinatario da mensagem
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

}

/* Lê do serial novas velocidades */
void receiveUSBdata(){
  int initCounter = 0;
    
  while(Serial.available()){
    /* Lê um caracter da mensagem */
    char character = Serial.read();

    /* Incrementa o contador se for 'B' */
    if(character == 'B') initCounter++;

    /* Se os três primeiros caracteres são 'B' então de fato é o início da mensagem */
    if(initCounter >= 3){
      VelocidadeSerial receber;
      
      /* Lê a mensagem até o caracter de terminação e a decodifica */
      Serial.readBytes((char*)(&receber), (size_t)sizeof(VelocidadeSerial));

      /* Faz o checksum */
      int16_t checksum = 0;
      for(int i=0 ; i<3 ; i++){
        checksum += receber.data.vl[i] + receber.data.vr[i];
      }

      /* Verifica o checksum */
      if(checksum == receber.checksum){
        /* Copia para o buffer global de velocidades */
        velocidades = receber.data;

        /* Reporta que deu certo */
        Serial.printf("%d\t%d\t%d\n", checksum, velocidades.vl[0], velocidades.vr[0]);
        
      }
      else {
        /* Devolve o checksum calculado se deu errado */
        for(uint16_t i=0 ; i<sizeof(VelocidadeSerial) ; i++){
          Serial.printf("%p ", ((char*)&receber)[i]);
        }
        Serial.println("");
        //Serial.printf("%p\t%p\t%p\n", checksum, velocidades.v[0], velocidades.w[0]);
      }

      /* Zera o contador */
      initCounter = 0;
    }
  }
}
