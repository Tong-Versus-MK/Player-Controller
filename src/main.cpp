#include <Arduino.h>
#include <math.h>
#include <Bounce2.h>
#include <WiFi.h>
#include <esp_now.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//fix this--------------fix this//
#define PLAYER_ID 0 // 0 = Tong, 1 = MK เปลี่ยนแค่ตัวนี้ตัวเดียวพอ
//fix this--------------fix this//


#define BOARD_SIZE 8

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
  {0,1,0,0,0,0,0,0},
  {0,1,1,1,0,0,1,0},
  {0,0,0,1,0,0,1,1},
  {1,1,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,1,0,0,1,0,0},
  {0,1,1,1,0,1,1,0},
  {0,0,0,0,0,0,1,0}
};

double damageModifier[] = {0,0.5,0.75,1,1.25,1.5,2};

void walk_mode();
void duel_mode();
void SendDataPlayer(int hit,int hp);

Bounce debouncer_up = Bounce(), debouncer_left = Bounce(), debouncer_down = Bounce(), debouncer_right = Bounce(), debouncer_btn = Bounce();
int move_count = 0;
int diced = 0;
int x = PLAYER_ID ? 7 : 0, y = PLAYER_ID ? 7 : 0;//tong 0,0 / MK 7,7
int wall_hit = 0;
const int player = PLAYER_ID;//tong 0 / MK 1
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

typedef struct struct_player{
    int hit;
    int hp;
}struct_player;

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
struct_player player_mess;

String success;
esp_now_peer_info_t peerInfo;
esp_now_peer_info_t peerInfo2;

uint8_t broadcastAddress[] = { 0x24, 0x0A, 0xC4, 0x9B, 0x8F, 0xEC };
uint8_t broadcastAddressPlayer[2][6] = {{ 0x3C, 0x71, 0xBF, 0x10, 0x5C, 0x3C },
                                        { 0x3C, 0x61, 0x05, 0x03, 0xA2, 0x74 }};


int waitForDisplay=0;

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
        
        if (recv_mess.stat_hp < hp && mode != 2) {
            waitForDisplay=1;
            OLED.clearDisplay();
            OLED.setCursor(0,0);
            OLED.setTextSize(2);
            OLED.println("GOT HIT");
            OLED.print(recv_mess.stat_hp - hp);
            OLED.print(" HP");
            OLED.display();
            delay(2000);
            waitForDisplay=0;
        }
        if (recv_mess.stat_hp > hp && mode != 2) {
            waitForDisplay=1;
            OLED.clearDisplay();
            OLED.setCursor(0,0);
            OLED.setTextSize(2);
            OLED.println("Obtain!");
            OLED.print("+");
            OLED.print(recv_mess.stat_hp - hp);
            OLED.print(" HP");
            OLED.display();
            delay(2000);
            waitForDisplay=0;
        }
        if (recv_mess.stat_atk > atk && mode != 2) {
            waitForDisplay=1;
            OLED.clearDisplay();
            OLED.setCursor(0,0);
            OLED.setTextSize(2);
            OLED.println("Obtain!");
            OLED.print("+");
            OLED.print(recv_mess.stat_atk - atk);
            OLED.print(" ATK");
            OLED.display();
            delay(2000);
            waitForDisplay=0;
        }
        hp = recv_mess.stat_hp;
        atk = recv_mess.stat_atk;
        SendDataPlayer(0,hp);
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
    delay(100);
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&pos_mess, sizeof(pos_mess));

    if (result == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
    }
}
/* ESP-NOW Communication Setup END*/

void SendDataPlayer(int hit,int hp){
    player_mess.hit=hit;
    player_mess.hp=hp;
    delay(100);
    esp_err_t result = esp_now_send(broadcastAddressPlayer[PLAYER_ID], (uint8_t*)&player_mess, sizeof(player_mess));

    if (result == ESP_OK) {
        Serial.println("Sent Hit with success");
    }
    else {
        Serial.println("Error sending the data");
    }
}

void writeOLED(char *text){
    OLED.clearDisplay(); // ลบภาพในหน้าจอทั้งหมด
    OLED.setTextColor(WHITE, BLACK);  //กำหนดข้อความสีขาว ฉากหลังสีดำ
    OLED.setCursor(45, 10); // กำหนดตำแหน่ง x,y ที่จะแสดงผล
    OLED.setTextSize(7); // กำหนดขนาดตัวอักษร
    OLED.print(text);
    OLED.display();
}

int diceRoll(int start,int end) {
    // int dice = (int)ceil(((double)esp_random() / 4294967295) * 6);
    if(start == end){
        return start;
    }
    char num[2];
    int dice;
    for(int i=0;i<20;i++){
        dice = (esp_random() % (end-start+1)) + start;
        sprintf(num,"%d",dice);
        writeOLED(num);
        delay(i*10);
    }

    writeOLED(num);
    delay(500);
    OLED.clearDisplay();
    OLED.display();
    delay(500);

    writeOLED(num);
    delay(500);
    OLED.clearDisplay();
    OLED.display();
    delay(500);
    
    writeOLED(num);
    delay(1000);

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
    }
    else {
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

    //esp_now_register_send_cb(OnDataSent);

    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    memcpy(peerInfo2.peer_addr, broadcastAddressPlayer[PLAYER_ID], 6);
    peerInfo2.channel = 0;
    peerInfo2.encrypt = false;

    if (esp_now_add_peer(&peerInfo2) != ESP_OK) {
        Serial.println("Failed to add peer 2");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
    OLED.clearDisplay();

    printMaze();
    pinMode(13,OUTPUT);
    digitalWrite(13,0);
}



int yourDice;
#define LED1 13
void loop() {
    if (turn==player) {
        digitalWrite(LED1,1);
    }
    else {
        digitalWrite(LED1,0);
    }
    if (mode == 0)
        walk_mode();
    else if (mode == 1)
        duel_mode();
    else if (mode == 2) {
        OLED.clearDisplay();
        OLED.setTextColor(WHITE, BLACK);
        OLED.setCursor(0, 0);
        OLED.setTextSize(5);
        if (turn == player) {
            OLED.print("WIN!!");
            printf("Congrats!! You Win!!\n");
        }
        else {
            OLED.print("NOOB!!");
            printf("You lose!! NOOB!\n");
        }
        OLED.display();
        delay(3000);
        OLED.clearDisplay();
        OLED.setCursor(0, 0);
        OLED.setTextSize(5);
        OLED.print("RESET");
        printf("Game Reset!\n");
        OLED.display();
        delay(2000);
        move_count = 0;
        diced = 0;
        x = PLAYER_ID ? 7 : 0, y = PLAYER_ID ? 7 : 0;   //tong 0,0 / MK 7,7
        wall_hit = 0;
        //turn = 0;
        //mode = 0;
        printMaze();
        while(mode!=0);
    }

    if(!waitForDisplay){
        OLED.clearDisplay(); // ลบภาพในหน้าจอทั้งหมด
        OLED.setTextColor(WHITE, BLACK);  //กำหนดข้อความสีขาว ฉากหลังสีดำ
        OLED.setCursor(0, 0); // กำหนดตำแหน่ง x,y ที่จะแสดงผล
        OLED.setTextSize(2); // กำหนดขนาดตัวอักษร
        OLED.println(player ? "MK" : "TONG");
        OLED.print("x: ");
        OLED.print(x);
        OLED.print(" y: ");
        OLED.println(y);
        OLED.setTextSize(1);
        OLED.println("-------------------");
        OLED.print("ATK : ");
        OLED.println(atk);
        // OLED.print("HP : ");
        // OLED.println(hp);
        OLED.print("Your Dice : ");
        OLED.println(yourDice);
        OLED.print("Move Count : ");
        OLED.println(move_count);
       
        // OLED.print("Turn Player : ");
        // OLED.println(turn);
        
        OLED.display(); // สั่งให้จอแสดงผล
    }
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
        move_count = diceRoll(1,6);
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
            wall_hit = 0;
            if (move_count == 0) {
                diced = 0;
                turn = !PLAYER_ID; // tong 1 / MK 0
            }
        }
    }
    if (debouncer_up.fell() || debouncer_left.fell() || debouncer_right.fell() || debouncer_down.fell() || debouncer_btn.fell()) {
        printMaze();
    }
}

int attackOption = 0;
/*
0 -> Always rolls 2
1 -> Rolls 1-4
2 -> Rolls 0-6
*/
void duel_mode() {
    debouncer_up.update();
    debouncer_left.update();
    debouncer_down.update();
    debouncer_right.update();
    debouncer_btn.update();

    if ((turn != player) && (debouncer_up.fell() || debouncer_left.fell() ||  debouncer_down.fell() ||  debouncer_right.fell() || debouncer_btn.fell())) {
        printf("Not your turn!\n");
    }
    else if (debouncer_up.fell()){
        attackOption = 0;
        printf("Option #0 | Test Roll: %d\n",diceRoll(2,2));
        OLED.clearDisplay();
        OLED.setCursor(0, 0);
        OLED.setTextSize(2);
        OLED.print("Dice Roll : ");
        OLED.println("2");
        OLED.println("Damage : ");
        OLED.print((int) ceil(atk*damageModifier[2]));
        OLED.print(" DMG");
        OLED.display();
        delay(2000);

    }
    else if (debouncer_left.fell()){
        attackOption = 1;
        OLED.clearDisplay();
        OLED.setCursor(0, 0);
        OLED.setTextSize(2);
        printf("Option #1\n");
        OLED.print("Dice Roll: ");
        OLED.println("1 - 3");
        OLED.println("Damage : ");
        OLED.print((int) ceil(atk*damageModifier[1]));
        OLED.print(" - ");
        OLED.print((int) ceil(atk*damageModifier[3]));
        OLED.print(" DMG");
        OLED.display();
        delay(2000);
    }
    else if (debouncer_right.fell()){
        attackOption = 2;
        OLED.clearDisplay();
        OLED.setCursor(0, 0);
        OLED.setTextSize(2);
        OLED.print("Dice Roll: ");
        OLED.println("0 - 7");
        printf("Option #2\n");
        OLED.println("Damage : ");
        OLED.print((int) ceil(atk*damageModifier[0]));
        OLED.print(" - ");
        OLED.print((int) ceil(atk*damageModifier[6]));
        OLED.print(" DMG");
        OLED.display();
        delay(2000);
    }
    else if (debouncer_btn.fell()) {
        switch(attackOption){
            case 0: move_count = diceRoll(2,2); break;
            case 1: move_count = diceRoll(1,4); break;
            case 2: move_count = diceRoll(0,6); break;
        }
        printf("Option (0-2): %d | Move: %d\n",attackOption,move_count);
        yourDice = move_count;
        SendData(player, x, y, -1, move_count);
        SendDataPlayer(1,hp);
        // printf("Attack with %d dmg\n", move_count);
        OLED.clearDisplay();
        OLED.setCursor(0, 0);
        OLED.setTextSize(2);
        OLED.println("Hit With");
        OLED.print((int) ceil(atk*damageModifier[move_count]));
        OLED.print(" DMG");
        OLED.display();
        delay(2000);
        while(turn==PLAYER_ID){
            if(mode == 2) break;
        }
        //turn = !PLAYER_ID; //tong 1 / MK 0
    }
}
