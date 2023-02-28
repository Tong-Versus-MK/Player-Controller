#include <Arduino.h>
 
#define RED 33
#define UP 19
#define LEFT 18
#define DOWN 5
#define RIGHT 17


int cnt = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Start...");

    pinMode(UP,INPUT_PULLUP);
    pinMode(LEFT,INPUT_PULLUP);
    pinMode(DOWN,INPUT_PULLUP);
    pinMode(RIGHT,INPUT_PULLUP);
}

int switchIsPresssed(int selected_swtich){
    if(selected_swtich != -1)
        return !digitalRead(selected_swtich);
    return !digitalRead(UP) || !digitalRead(LEFT) || !digitalRead(DOWN) || !digitalRead(RIGHT);
}

void loop() {
    while(!switchIsPresssed(-1))
        delay(50);
    if(switchIsPresssed(UP)){
        Serial.println("UP");
    }
    if(switchIsPresssed(LEFT)){
        Serial.println("LEFT");
    }
    if(switchIsPresssed(DOWN)){
        Serial.println("DOWN");
    }
    if(switchIsPresssed(RIGHT)){
        Serial.println("RIGHT");
    }
    while(switchIsPresssed(-1))
        delay(50);
}