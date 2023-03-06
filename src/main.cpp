#include <Arduino.h>
#include <math.h>
#include <Bounce2.h>
#include <WiFi.h>
#include <esp_now.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BOARD_SIZE 6

#define UP 27
#define LEFT 26
#define DOWN 25
#define RIGHT 33

#define BUTTON 19

#define SCREEN_WIDTH 128 // pixel ความกว้าง
#define SCREEN_HEIGHT 64 // pixel ความสูง 
#define OLED_RESET     -1
Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int MAZE[BOARD_SIZE][BOARD_SIZE] = {
    {0,1,0,0,0,0},
    {0,1,0,1,1,0},
    {0,0,0,1,1,0},
    {1,1,0,0,0,0},
    {0,0,0,0,1,0},
    {0,0,1,0,1,0}
};

void walk_mode();
void duel_mode();

Bounce debouncer_up = Bounce(), debouncer_left = Bounce(), debouncer_down = Bounce(), debouncer_right = Bounce(), debouncer_btn = Bounce();
int move_count = 0;
int diced = 0;
int x = 5, y = 5;//tong 0,0 / MK 5,5
int wall_hit = 0;
const int player = 1;//tong 0 / MK 1
int turn = 0;
int mode = 0;
int hp = 30;
int atk = 0;

/* ESP-NOW Communication Setup Start*/
typedef struct struct_message {
    int player;
    int x;
    int y;
    int wall_hit;
    int move_count;
} struct_message;

//turn 0=Tong 1=MK
//mode 0=walk 1=Duel 2=End
typedef struct recv_message {
    int turn;
    int mode;
    int stat_owner;
    int stat_hp;
    int stat_atk;

} recv_message;

struct_message pos_mess;
recv_message recv_mess;

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

// Callback when data is recieve
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
    memcpy(&recv_mess, incomingData, sizeof(recv_mess));
    //Serial.print("Bytes received: ");
    //Serial.println(len);

    if (mode == 0 && recv_mess.mode == 1)
        printf("IT'S TIME FOR D..Du..Duel!!!\n");
    turn = recv_mess.turn;
    if (turn == player) {
        if (recv_mess.mode == 0 && mode == 1)
            printf("Your turn please roll your dice to walk.\n");
        else if (recv_mess.mode == 1 && mode == 0)
            printf("Your turn please roll your dice to attack.\n");
    }
    mode = recv_mess.mode;

    if (recv_mess.stat_owner == player || recv_mess.stat_owner == 2) {
        hp = recv_mess.stat_hp;
        atk = recv_mess.stat_atk;
    }
    printf("Player %d\n", player);
    printf("HP: %d\n", hp);
    printf("ATK: %d\n", atk);
    printf("Turn: %d\n", turn);
    printf("Mode: %d\n", mode);
}

void SendData(int player, int x, int y, int wall_hit, int move_count) {
    pos_mess.player = player;
    pos_mess.x = x;
    pos_mess.y = y;
    pos_mess.wall_hit = wall_hit;
    pos_mess.move_count = move_count;//in duel mode this value is dice value for attack
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&pos_mess, sizeof(pos_mess));

    if (result == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
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
    
    if (!OLED.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // สั่งให้จอ OLED เริ่มทำงานที่ Address 0x3C
    Serial.println("SSD1306 allocation failed");
    } else {
      Serial.println("ArdinoAll OLED Start Work !!!");
    }

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

    esp_now_register_recv_cb(OnDataRecv);

    printMaze();
}

int yourDice;

void loop() {
    if (mode == 0)
        walk_mode();
    else if (mode == 1)
        duel_mode();
    else if (mode == 2) {
        if (turn == player)
            printf("Congrats!! You Win!!\n");
        else
            printf("You lose!! NOOB!\n");
        printf("Game Reset!\n");
        move_count = 0;
        diced = 0;
        x = 5; //tong 0 / MK 5
        y = 5; //tong 0 / MK 5
        wall_hit = 0;
        turn = 0;
        mode = 0;
        printMaze();
    }

    
    OLED.clearDisplay(); // ลบภาพในหน้าจอทั้งหมด
    OLED.setTextColor(WHITE, BLACK);  //กำหนดข้อความสีขาว ฉากหลังสีดำ
    OLED.setCursor(0, 0); // กำหนดตำแหน่ง x,y ที่จะแสดงผล
    OLED.setTextSize(2); // กำหนดขนาดตัวอักษร
    OLED.print("Player : ");
    OLED.println(player);
    OLED.setTextSize(1);
    OLED.print("ATK : ");
    OLED.println(atk);
    OLED.print("HP : ");
    OLED.println(hp);
    OLED.print("Your Dice : ");
    OLED.println(yourDice);
    OLED.print("Move Count : ");
    OLED.println(move_count);
    OLED.print("Turn Player : ");
    OLED.println(turn);
    OLED.display(); // สั่งให้จอแสดงผล
}


void walk_mode() {
    debouncer_up.update();
    debouncer_left.update();
    debouncer_down.update();
    debouncer_right.update();
    debouncer_btn.update();

    if ((turn != player) && (debouncer_btn.fell() || debouncer_up.fell() || debouncer_left.fell() || debouncer_right.fell() || debouncer_down.fell())) {
        printf("Not your turn!\n");
    }
    else if (debouncer_btn.fell() && !diced) {
        diced = 1;
        move_count = diceRoll();
        yourDice = move_count;
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
                    wall_hit = 1;
                }
                else {
                    y--;
                }
            }
            if (debouncer_left.fell()) {
                if (x <= 0 || MAZE[y][x - 1]) {
                    Serial.print("You hit the wall! -1 HP | ");
                    wall_hit = 1;
                }
                else {
                    x--;
                }
            }
            if (debouncer_down.fell()) {
                if (y >= BOARD_SIZE - 1 || MAZE[y + 1][x]) {
                    Serial.print("You hit the wall! -1 HP | ");
                    wall_hit = 1;
                }
                else {
                    y++;
                }
            }
            if (debouncer_right.fell()) {
                if (x >= BOARD_SIZE - 1 || MAZE[y][x + 1]) {
                    Serial.print("You hit the wall! -1 HP | ");
                    wall_hit = 1;
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
            SendData(player, x, y, wall_hit, move_count);
            if (move_count == 0) {
                diced = 0;
                wall_hit = 0;
                turn = 0; // tong 1 / MK 0
            }
        }
    }
    if (debouncer_up.fell() || debouncer_left.fell() || debouncer_right.fell() || debouncer_down.fell() || debouncer_btn.fell()) {
        printMaze();
    }
}

void duel_mode() {
    //debouncer_up.update();
    //debouncer_left.update();
    //debouncer_down.update();
    //debouncer_right.update();
    debouncer_btn.update();

    if ((turn != player) && debouncer_btn.fell()) {
        printf("Not your turn!\n");
    }
    else if (debouncer_btn.fell()) {
        move_count = diceRoll();
        yourDice = move_count;
        printf("Attack with %d dmg\n", move_count);
        SendData(player, x, y, -1, move_count);
        turn = 0; //tong 1 / MK 0
    }
}
