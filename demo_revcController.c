#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

typedef struct struct_message {
  int player;
  int x;
  int y;
  int wall_hit;
} struct_message;

struct_message income_mess;
int player;
int pos_x;
int pos_y;
int wall_hit;
volatile int walked;

String success;
esp_now_peer_info_t peerInfo;

uint8_t broadcastAddress[] = { 0x24, 0x6F, 0x28, 0x50, 0xA6, 0x78 };


void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&income_mess, incomingData, sizeof(income_mess));
  Serial.print("Bytes received: ");
  Serial.println(len);
  player = income_mess.player;
  pos_x = income_mess.x;
  pos_y = income_mess.y;
  wall_hit = income_mess.wall_hit;
  Serial.print("Player: ");
  Serial.println(player);
  Serial.print("Pos_x: ");
  Serial.println(pos_x);
  Serial.print("Pos_y: ");
  Serial.println(pos_y);
  Serial.print("Wall_Hit: ");
  Serial.println(wall_hit);
  walked = 1;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  Serial.println("Start...");
  Serial.println("Wait for walk..");
  walked = 0;
  while (!walked)
    ;
  // put your main code here, to run repeatedly:
}
