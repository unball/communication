#include <SPI.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

/* Definições */
#define NUMBER_OF_ROBOTS 3
#define MAX_POWER 10.5  //TODO: Testar valores
#define WIFI_CHANNEL 12  //TODO: Testar valores

//Endereço de broadcast,FF pois envia para todos
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* Estrutura para a mensagem a ser transmitida para o robô via wi-fi */
struct Velocidade{
  int16_t vl[3];
  int16_t vr[3];
};

struct MensagemWifi{
  int16_t id;
  int16_t vl;
  int16_t vr;
  int32_t checksum;
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

    // Envia via Wi-Fi caso a mensagem seja nova
    if (newData){
      sendWifi();
      newData = false;
    }

    // Acende o LED se enviou mensagens em menos de 5ms
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
    MensagemWifi vel = {.id = i, .vl = velocidades.vl[i], .vr = velocidades.vr[i]};
    
    //Envia a mensagem usando o ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &vel, sizeof(MensagemWifi));
    delay(3);
  }
  
  if(result){
    lastOK = millis();
  }
}

/* Configura o wi-fi */
void wifiSetup(){
  //Coloca o dispositivo no modo Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.setOutputPower(MAX_POWER);

  //Inicializa o ESP-NOW
  if (esp_now_init() != 0) {
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  //Registra o destinatario da mensagem
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

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
        /* Copia para o buffer global de velocidades e indica que a mensagem é recente */
        velocidades = receber.data;

        newData = true;

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
