#include <Bounce2.h>

/*
 * Keypad system to replace the lock system for the fridge door?
 * There are 12 buttons, each associated with a unique letter 
 * There are 5 leds, to indicate the current state of the input sequence
 * Only one led is on at a time, the leds dance transitioning from the last state to the first
 * if correct, otherwise the last led is lit for one second.
 * 
 * Reset by holding the buttons O and S aat the same time for three seconds
 * Once this door has been unlocked, the keypad is "disabled" for the players and the door 
 * remains unlocked until a staff reset
 * 
 * Mapping of letters to pin numbers
 * A, 8, 1    O: 
 * T: 7, 0    S: 9, 2
 * I:         K:
 * L:         R:
 * U:         M:
 * Y: 10,3    N:
 * 
 * All letters not part of the correct sequence share the last pin (11) (i.e they are set up in parallel)
 */
 

#define RELAY_PIN 13
#define NUM_LEDS 5
#define NUM_BUTTON_PINS 5 // Uno does not have that many pins, so all invalid buttons share a pin
#define SEQUENCE_LENGTH 5
#define LOCKOUT_TIME 300
#define DEBOUNCE_TIME 100
#define PLAYER_LOCKOUT_TIME 1000 //  time (s) the last led is displayed for 
#define RESET_BUTTON 0 // index of the button to be held
#define OTHER_RESET_BUTTON 3 // both buttons are to be held for the full reset
#define RESET_BUTTON_HOLD_TIME 3000 // time (ms) that the reset buttons must be held for
#define FLICKER 100 // time (ms) that each led is on for during the dance
#define NUM_FLICKERS 5

const int ledPins[NUM_LEDS] = {2,3,4,5,6};
const int buttonPins[NUM_BUTTON_PINS] = {7,8,9,10,11};
const int correctSequence[SEQUENCE_LENGTH] = {7,8,9,7,10}; 

int currentSequence[SEQUENCE_LENGTH] = {0,0,0,0,0};
int sequenceCounter = 0;

Bounce debounce[NUM_BUTTON_PINS];

int long previousTime = 0;

int ledFlags[NUM_LEDS] = {0,0,0,0,0};

bool isDoorUnlocked = false;

void setup() {

  for (int i = 0; i < NUM_BUTTON_PINS; i++){
    debounce[i] = Bounce();
    debounce[i].attach(buttonPins[i]);
    debounce[i].interval(DEBOUNCE_TIME);   
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  for (int i = 0; i < NUM_LEDS; i++){
     pinMode(ledPins[i], OUTPUT);
     digitalWrite(ledPins[i], LOW);
  }

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // turns the magnet on to lock the door

  Serial.begin(9600);

}

void loop() {

  for (int i = 0; i < NUM_BUTTON_PINS; i++){
    debounce[i].update();   
  }
  
  if (isDoorUnlocked){  
    if (debounce[RESET_BUTTON].read() == LOW && debounce[OTHER_RESET_BUTTON].read() == LOW){
      Serial.println("A ,O Being held");
      delay(RESET_BUTTON_HOLD_TIME);
      if (debounce[RESET_BUTTON].read() == LOW && debounce[OTHER_RESET_BUTTON].read() == LOW){
        reset();
        previousTime = millis(); // temporarily ignores input, or else registers a button press 
      }
    }
    return; 
  }

  if (millis() - previousTime < LOCKOUT_TIME){
      return; 
  }

  // poll the state of the buttons for whichever was pushed
  for (int i = 0; i < NUM_BUTTON_PINS; i++){
    if (debounce[i].read() == LOW){
      pushButton(i);
      previousTime = millis();
      break;
    }
  } 

  // light up the leds according to the flags
  for (int i = 0; i < NUM_LEDS; i++){
    digitalWrite(ledPins[i], ledFlags[i]);  
  }

}

boolean checkSequence(){
  for (int i = 0; i < SEQUENCE_LENGTH; i++){
    if (currentSequence[i] != correctSequence[i]){
      return false;
    }
  }
  return true;
}

/*
 * Input: pin number of the button that was pushed (7 to 12) 
 * Stores the integer in the current sequence 
 * Adjusts the led flags
 * On the 5th button press, check the validity of the sequence and 
 * call the win or reset function
 */
 
void pushButton(int i){ 
  
  currentSequence[sequenceCounter] = i;
  Serial.print(i);  
    
  if (sequenceCounter == NUM_LEDS - 1){
      if (checkSequence()){
        Serial.println("Correct Sequence");
        win();
      }else{
        Serial.println("Incorrect Sequence");
        digitalWrite(ledPins[NUM_LEDS -2], LOW);
        digitalWrite(ledPins[NUM_LEDS -1], HIGH);
        delay(PLAYER_LOCKOUT_TIME); // light up the last led for a second
        reset();
      }
  }else{    
      for (int i = 0; i < NUM_LEDS; i++){
        ledFlags[i] = LOW;
      }   
      ledFlags[sequenceCounter] = HIGH;
      sequenceCounter++; 
  }
  
}

/*
 * Reset the ledflags, the current sequence input and other variables.
 * Turn on the magnetic lock.
 */
 
void reset(){
  for (int i = 0; i < SEQUENCE_LENGTH; i++){
    currentSequence[i] = 0;
    ledFlags[i] = LOW; 
  }
  sequenceCounter = 0;
  isDoorUnlocked = false;
  digitalWrite(RELAY_PIN, HIGH);
  delay(PLAYER_LOCKOUT_TIME);
}

/*
 * Unlock the magnetic lock and make the leds dance, then reset the led flags.
 */
 
void win(){
  dance();
  for (int i = 0; i < NUM_LEDS; i++){
    ledFlags[i] = LOW; 
  }
  isDoorUnlocked = true;
  digitalWrite(RELAY_PIN, LOW);
}

/*
 * Only one led can be on at a time so just light them up one at a time
 * in sequence
 */
 
void dance(){
  for (int i = 0; i < NUM_FLICKERS; i++){
    digitalWrite(ledPins[NUM_LEDS -1], LOW);
    digitalWrite(ledPins[0], HIGH);
    delay(FLICKER); 
    for (int i = 1; i < NUM_LEDS; i++){ 
      digitalWrite(ledPins[i-1], LOW);
      digitalWrite(ledPins[i],HIGH);
      delay(FLICKER);
    }
  }
}
