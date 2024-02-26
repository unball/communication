#include <cstdint>

#define PID_TUNNER true
#define MAX_POWER 10.0  //TODO: Test values
#define WIFI_CHANNEL 12  //TODO: Test values
#define TWIDDLE_ON false
#define CONTROL_ON true

enum Mode {
  twiddle = 0,
  control = 1,
  no_control = 2,
};


/* Estrutura para a mensagem a ser transmitida para o robô via wi-fi */
struct RobotMessage{
  int16_t v[3];
  int16_t w[3];
};

/* Estrutura para a mensagem a ser recebida do USB */
struct SerialMessage {
  RobotMessage data;
  int16_t checksum;
};

/* Broadcast address, sends to everyone */
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
struct snd_message{
  uint8_t control;
  uint8_t id;
  int16_t kp;
  int16_t ki;
  int16_t kd;
  int16_t v;
  int16_t w;
};


struct rcv_message{
  uint8_t id;  
  float error;
};

struct SerialConstants {
  int16_t id;
  int16_t kp;
  int16_t ki;
  int16_t kd;
};

/* Control constants for robot 0, 1 and 2, update values after running Twiddle algorithm */
struct ControlConstants {
  const int16_t kp[3] = {(int16_t)(0.159521 * 100), (int16_t)(0.159521 * 100), (int16_t)(0.159521 * 100)};
  const int16_t ki[3] = {(int16_t)(0.016864 * 100), (int16_t)(0.016864 * 100), (int16_t)(0.016864 * 100)};
  const int16_t kd[3] = {(int16_t)(0.016686 * 100), (int16_t)(0.016686 * 100), (int16_t)(0.016686 * 100)};
};



