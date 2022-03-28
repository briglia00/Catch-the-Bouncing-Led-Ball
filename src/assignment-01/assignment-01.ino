#include <avr/sleep.h>
#include <avr/power.h>
#define EI_ARDUINO_INTERRUPTED_PIN // to enable pin states functionality 
#include <EnableInterrupt.h>

#define POT_PIN A0
#define LED_RED 6
#define LED_G1 8
#define LED_G2 9
#define LED_G3 10
#define LED_G4 12
#define BUT2 2
#define BUT3 3
#define BUT4 4
#define BUT1 5

#define base_led_delay 200
#define base_reaction_time 1500
#define factor_multiplier 1.0

volatile int score;

String STATE;
int difficulty;
int currentIntensity;
int currentLed;
int index;
int dirLed;
int led_delay;
float reaction_time;
int max_led_bouncing_time;
int min_led_bouncing_time;
unsigned int random_time;
unsigned long current_time;

int leds[] = {LED_G1,LED_G2,LED_G3,LED_G4};
int buttons[] = {BUT1,BUT2,BUT3,BUT4};
int arraysize = 4;
int fade = 5;

void setup() {
  STATE = "WAITING";
  score = 0;
  led_delay = base_led_delay;
  current_time = millis();
  reaction_time = base_reaction_time;

  min_led_bouncing_time = 2;
  max_led_bouncing_time = 6;
  
  currentIntensity = 0;
  currentLed = leds[0];
  index = 0;
  dirLed = 1;
  
  enableInterrupt(BUT1, startGame, RISING);
  
  Serial.begin(9600);
  Serial.println("Welcome to the Catch the Bouncing Led Ball Game. Press Key T1 to Start");
}

void loop() {
  if(STATE == "WAITING"){
    analogWrite(LED_RED, currentIntensity);
    currentIntensity = currentIntensity + fade;
    if (currentIntensity >= 255 || currentIntensity <= 0){
      fade = -fade; 
    }
    delay(30);
    noInterrupts();
    unsigned long temp_current_time = current_time;
    interrupts();
    if(temp_current_time + 10000 < millis()){
      sleepMode();
    }
  }
  
  else if(STATE == "START"){
    digitalWrite(currentLed, HIGH);
    delay(led_delay);
    digitalWrite(currentLed, LOW);
    if((index == 0 && dirLed == -1) || (index == (arraysize - 1) && dirLed == 1)){
      dirLed = -dirLed;
    }
    index = index + dirLed;
    currentLed = leds[index];
    
    noInterrupts();
    unsigned long temp_current_time = current_time;
    interrupts();
    
    if(temp_current_time + (random_time*1000) < millis()){
      stopGame();
    }
  }

  else if(STATE == "LED_STOP"){
    if(millis() > reaction_time + current_time){
      lostGame();
    }
  }
  
  else if(STATE == "LOST"){
    noInterrupts();
    unsigned long temp_current_time = current_time;
    interrupts();
    if(temp_current_time + 10000 < millis()){
      enableInterrupt(buttons[0], startGame, RISING);
      Serial.println("Welcome to the Catch the Bouncing Led Ball Game. Press Key T1 to Start");
      currentLed = leds[0];
      index = 0;
      dirLed = 1;
      current_time = millis();
      STATE = "WAITING";
    }
  }
  
  else if(STATE == "WON"){
    noInterrupts();
    score = score + 1;
    difficulty = difficulty + 1;
    interrupts();
    Serial.println("New point! Score: " + (String) score);
    delay(150);
    startGame();
  }
}

void startGame(){
  noInterrupts();
  String tempSTATE = STATE;
  interrupts();
  if(tempSTATE != "WAITING" && tempSTATE != "WON"){
    return NULL;
  }
  if(tempSTATE == "WAITING"){
    difficulty = map(analogRead(POT_PIN), 0, 1023, 1, 8);
  }
  random_time = random(min_led_bouncing_time, max_led_bouncing_time);
  STATE = "START";
  current_time = millis();
  refreshDifficulty();
  analogWrite(LED_RED, 0);
  for(int i = 0; i < arraysize; i++){
    disableInterrupt(buttons[i]);
    enableInterrupt(buttons[i], checkClick, RISING);
  }
  Serial.println("Go!");
}

void sleepMode(){
  for(int i = 0; i < arraysize; i++){
    disableInterrupt(buttons[i]);
    enableInterrupt(buttons[i], wakeUp, RISING);
  }
  STATE="SLEEP";
  analogWrite(LED_RED, 0);
  Serial.println("Entering sleep mode!");
  Serial.flush();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
}

void wakeUp(){
  sleep_disable();
  noInterrupts();
  String tempSTATE = STATE;
  interrupts();
  if(tempSTATE != "SLEEP"){
    return NULL;
  }
  STATE="WAITING";
  for(int i = 0; i < arraysize; i++){
    disableInterrupt(buttons[i]);
  }
  delay(150);
  enableInterrupt(BUT1, startGame, RISING);
  current_time = millis();
  Serial.println("Exiting sleep mode!");
}

void stopGame(){
  noInterrupts();
  String tempSTATE = STATE;
  interrupts();
  if(tempSTATE != "START"){
    return NULL;
  }
  STATE = "LED_STOP";
  current_time = millis();
  digitalWrite(currentLed, HIGH);
}

void lostGame(){
  Serial.println("Game Over. Final Score: " + (String) score);
  analogWrite(LED_RED, 0);
  analogWrite(LED_RED, 255);
  delay(50);
  analogWrite(LED_RED, 0);
  delay(50);
  analogWrite(LED_RED, 255);
  delay(50);
  analogWrite(LED_RED, 0);
  score = 0;
  current_time = millis();
  STATE = "LOST";
}

void checkClick(){
  noInterrupts();
  String tempSTATE = STATE;
  interrupts();

  for(int i = 0; i < arraysize; i++){
    disableInterrupt(buttons[i]);
    digitalWrite(leds[i], LOW);
  }
  
  if(tempSTATE == "START"){
    lostGame();
    return NULL;
  } else if(tempSTATE != "LED_STOP"){
    return NULL;
  }
  
  noInterrupts();
  int delayed_reaction = millis() - current_time;
  interrupts();
  if(delayed_reaction < reaction_time && arduinoInterruptedPin == buttons[index]){
    STATE = "WON";
  } else {
    lostGame();
  }
}

void refreshDifficulty(){
  noInterrupts();
  int temp_diff = difficulty;
  interrupts();
  
  reaction_time = base_reaction_time / (factor_multiplier + (temp_diff * 0.1));
  led_delay = base_led_delay / (factor_multiplier + (temp_diff * 0.1));

  if(temp_diff >= 9){
    min_led_bouncing_time = 1;
    max_led_bouncing_time = 3;
  }
}
