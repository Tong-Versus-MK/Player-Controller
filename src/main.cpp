#include <Arduino.h>
#include <math.h>
#include <Bounce2.h>
#include <WiFi.h>
#include <esp_now.h>

#define BOARD_SIZE 6

#define UP 27
#define LEFT 26
#define DOWN 25
#define RIGHT 33

#define BUTTON 19

int MAZE[BOARD_SIZE][BOARD_SIZE] = {
    {0,1,0,0,0,0},
    {0,1,0,1,1,0},
    {0,0,0,1,1,0},
    {1,1,0,0,0,0},
    {0,0,0,0,1,0},
    {0,0,1,0,1,0}
};

Bounce debouncer_up = Bounce(), debouncer_left = Bounce(), debouncer_down = Bounce(), debouncer_right = Bounce(), debouncer_btn = Bounce();
int move_count = 0;
int diced = 0;
int x = 0, y = 0;
int wall_hit = 0;
const int player = 0;

/* ESP-NOW Communication Setup Start*/
typedef struct struct_message {
    int player;
    int x;
    int y;
    int wall_hit;
} struct_message;

struct_message pos_mess;

String success;
esp_now_peer_info_t peerInfo;

uint8_t broadcastAddress[] = { 0x24, 0x0A, 0xC4, 0x9B, 0x8F, 0xEC };

// Callback when data is sent
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    if (status == 0) {
        success = "Delivery Success :)";
    }
    else {
        success = "Delivery Fail :(";
    }
}
/* ESP-NOW Communication Setup END*/

int diceRoll() {
    int dice = (int)ceil(((double)esp_random() / 4294967295) * 6);
    return dice;
}

int switchIsPresssed(int selected_swtich) {
    if (selected_swtich != -1)
        return !digitalRead(selected_swtich);
    return !digitalRead(UP) || !digitalRead(LEFT) || !digitalRead(DOWN) || !digitalRead(RIGHT);
}

void printMaze() {
    Serial.println("# # # # # # # #");
    for (int i = 0;i < BOARD_SIZE;i++) {
        Serial.print("# ");
        for (int j = 0;j < BOARD_SIZE;j++) {
            if (MAZE[i][j]) {
                Serial.print("# ");
            }
            else if (i == y && j == x) {
                Serial.print("x ");
            }
            else {
                Serial.print(". ");
            }
        }
        Serial.println("#");
    }
    Serial.println("# # # # # # # #");
}

void setup() {
    Serial.begin(115200);
    Serial.println("Start...");

    debouncer_up.attach(UP, INPUT_PULLUP);
    debouncer_left.attach(LEFT, INPUT_PULLUP);
    debouncer_down.attach(DOWN, INPUT_PULLUP);
    debouncer_right.attach(RIGHT, INPUT_PULLUP);
    debouncer_btn.attach(BUTTON, INPUT_PULLUP);

    debouncer_up.interval(25);
    debouncer_left.interval(25);
    debouncer_down.interval(25);
    debouncer_right.interval(25);
    debouncer_btn.interval(25);

    WiFi.mode(WIFI_MODE_STA);
    Serial.println(WiFi.macAddress());

    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent);

    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    printMaze();
}

void loop() {
    debouncer_up.update();
    debouncer_left.update();
    debouncer_down.update();
    debouncer_right.update();
    debouncer_btn.update();

    if (debouncer_btn.fell() && !diced) {
        diced = 1;
        move_count = diceRoll();
        Serial.print("You got ");
        Serial.print(move_count);
        Serial.println(" count");

    }
    else if ((debouncer_up.fell() || debouncer_left.fell() || debouncer_right.fell() || debouncer_down.fell())) {
        if (!diced) {
            Serial.println("Roll dice first!");
        }
        else {
            if (debouncer_up.fell()) {
                if (y <= 0 || MAZE[y - 1][x]) {
                    Serial.print("You hit the wall! -1 HP | ");
                    wall_hit++;
                }
                else {
                    y--;
                }
            }
            if (debouncer_left.fell()) {
                if (x <= 0 || MAZE[y][x - 1]) {
                    Serial.print("You hit the wall! -1 HP | ");
                    wall_hit++;
                }
                else {
                    x--;
                }
            }
            if (debouncer_down.fell()) {
                if (y >= BOARD_SIZE - 1 || MAZE[y + 1][x]) {
                    Serial.print("You hit the wall! -1 HP | ");
                    wall_hit++;
                }
                else {
                    y++;
                }
            }
            if (debouncer_right.fell()) {
                if (x >= BOARD_SIZE - 1 || MAZE[y][x + 1]) {
                    Serial.print("You hit the wall! -1 HP | ");
                    wall_hit++;
                }
                else {
                    x++;
                }
            }

            move_count--;
            Serial.print("(");
            Serial.print(x);
            Serial.print(", ");
            Serial.print(y);
            Serial.print(") | ");
            Serial.print(move_count);
            Serial.println(" Move left");

            if (move_count == 0) {
                pos_mess.player = player;
                pos_mess.x = x;
                pos_mess.y = y;
                pos_mess.wall_hit = wall_hit;
                esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&pos_mess, sizeof(pos_mess));

                if (result == ESP_OK) {
                    Serial.println("Sent with success");
                }
                else {
                    Serial.println("Error sending the data");
                }
            }
        }
    }
    if (debouncer_up.fell() || debouncer_left.fell() || debouncer_right.fell() || debouncer_down.fell() || debouncer_btn.fell()) {
        printMaze();
    }
}
