#include <Arduino.h>
#include <math.h>
#include <Bounce2.h>

#define BOARD_SIZE 6

#define UP 27
#define LEFT 26
#define DOWN 25
#define RIGHT 33

#define BUTTON 19


Bounce debouncer_up = Bounce(), debouncer_left = Bounce(), debouncer_down = Bounce(), debouncer_right = Bounce(), debouncer_btn = Bounce();
int move_count = 0;
int diced = 0;
int x = 0,y = 0;

int diceRoll(){
  int dice = (int)ceil(((double)esp_random()/4294967295)*6);
  return dice;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Start...");

    debouncer_up.attach(UP,INPUT_PULLUP);
    debouncer_left.attach(LEFT,INPUT_PULLUP);
    debouncer_down.attach(DOWN,INPUT_PULLUP);
    debouncer_right.attach(RIGHT,INPUT_PULLUP);
    debouncer_btn.attach(BUTTON,INPUT_PULLUP);

    debouncer_up.interval(25);
    debouncer_left.interval(25);
    debouncer_down.interval(25);
    debouncer_right.interval(25);
    debouncer_btn.interval(25);
}

int switchIsPresssed(int selected_swtich){
    if(selected_swtich != -1)
        return !digitalRead(selected_swtich);
    return !digitalRead(UP) || !digitalRead(LEFT) || !digitalRead(DOWN) || !digitalRead(RIGHT);
}

void loop() {
    debouncer_up.update();
    debouncer_left.update();
    debouncer_down.update();
    debouncer_right.update();
    debouncer_btn.update();

    if(debouncer_btn.fell()){
        diced = 1;
        move_count = diceRoll();
        Serial.print("You got ");
        Serial.print(move_count);
        Serial.println(" count");
    }
    else if((debouncer_up.fell() || debouncer_left.fell() || debouncer_right.fell() || debouncer_down.fell())){
        if(!diced){
            Serial.println("Roll dice first!");
        }
        else{
            if(debouncer_up.fell()){
                if(y <= 0){
                    Serial.print("You hit the wall! -1 HP | ");
                }
                else{
                    y--;
                }
            }
            if(debouncer_left.fell()){
                if(x <= 0){
                    Serial.print("You hit the wall! -1 HP | ");
                }
                else{
                    x--;
                }
            }
            if(debouncer_down.fell()){
                if(y >= BOARD_SIZE - 1){
                    Serial.print("You hit the wall! -1 HP | ");
                }
                else{
                    y++;
                }
            }
            if(debouncer_right.fell()){
                if(x >= BOARD_SIZE - 1){
                    Serial.print("You hit the wall! -1 HP | ");
                }
                else{
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

            if(move_count == 0){
                diced = 0;
            }
        }
    }
}
