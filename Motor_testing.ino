#include <Encoder.h>

const byte motor = 6;  //Motors
const byte dir1 = 7;
const byte dir2 = 8;

Encoder Enc(2, 3); // Initialize encoder on interrupt-capable pins 2 and 3

float raw = 0;
float position = 0;
int speed;

int CPR = 1920;


void setup() {
  Serial.begin(9600);

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  pinMode(motor, OUTPUT);
  pinMode(dir1, OUTPUT);
  pinMode(dir2, OUTPUT);
  pinMode(A0, INPUT);

  digitalWrite(dir1, HIGH);
  digitalWrite(dir2, LOW);
}

void loop() {

  raw = Enc.read();
  position = (raw / CPR)*(360);
  /*if(position >= 360){
    position -= 360;
    raw = -position*1920;
    Enc.write(raw);
  }*/

  Serial.print(" | Encoder: ");
  Serial.println(raw);

  Serial.print(" | Position: ");
  Serial.println(position);
  
  delay(100);
}

/*
void setup() {
  Serial.begin(9600);

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  pinMode(motor, OUTPUT);  //Motors
  pinMode(dir1, OUTPUT);
  pinMode(dir2, OUTPUT);
  pinMode(A0, INPUT);

  digitalWrite(dir1, HIGH);
  digitalWrite(dir2, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  speed = map(analogRead(A0), 1023, 0, 50, 255);
  analogWrite(motor, speed);
  
  position = Enc.read();  // Read directly in loop
  Serial.println(position);
  delay(100);
}
*/
