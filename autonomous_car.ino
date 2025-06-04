#define maximum_distance 200
int distance = 100;
const int trig =10;
const int echo = 11;

const int state_line = 0;

const int motorA1 = 9;
const int motorA2 = 6;
const int motorB1 = 5;
const int motorB2 = 3;

const int Pin_ss1 = A0;
const int Pin_ss2 = A1;
const int Pin_ss3 = A2;
const int Pin_ss4 = A3;
const int Pin_ss5 = A4;
int sensorValue = 0;
int outputValue = 0;
int IN_line, In_line_last, mode;
byte ss1, ss2, ss3, ss4, ss5, x1;
String tt;
int td3 = 160;
int td2 = 160;
int td1 = 100;
long duration;
unsigned long thoigian;
int khoangcach;

void setup() {
    pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  pinMode(Pin_ss1, INPUT);
  pinMode(Pin_ss2, INPUT);
  pinMode(Pin_ss3, INPUT);
  pinMode(Pin_ss4, INPUT);
  pinMode(Pin_ss5, INPUT);

  Serial.begin(9600);

}


bool isTurning = false;

void loop() {


  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  IN_line = In_SenSor();

  duration = pulseIn(echo, HIGH);
  distance = duration * 0.034 / 2; 
  khoangcach = int(duration / 2 / 29.412);

if (distance > 0 && distance < 15) {
  forward(200);
  delay(1000);     
} else {  
  if (IN_line == 0b00000000) IN_line = In_line_last;
  switch (IN_line) {
  case 0b00000100:
       forward(td1);
       break;

     case 0b00000110:
       Turn_right_90(td2);
       break;

   case 0b00000111:
       Turn_right_180(td3);
       break;
          case 0b00001111:
       Turn_right_180(td3);
       break;

          case 0b00000101:
       Turn_right_180(td3);
       break;


     case 0b00000011:
       Turn_right_180(td2);
       delay(150);
       break;


     case 0b00000001:
       Turn_right_180(td2);
       delay(150);
       break;


     case 0b00011100:
       Turn_left_180(td3);
       delay(150);
       break;

       
     case 0b00011110:
       Turn_left_180(td3);
       delay(150);
       break;


     case 0b00010100:
       Turn_left_180(td3);
       delay(150);
       break;


     case 0b00011000:
       Turn_left_180(td2);
       delay(150);
       break;


     case 0b00001100:
       Turn_left_90(td2);
       break;


     case 0b00010000:
       Turn_left_180(td3);
       break;



     case 0b00011111:
       Robot_stop();
       break;

       
     default:
       forward(td1);
       break;
   }

   In_line_last = IN_line;
}

  Serial.print("Khoảng cách: ");
  Serial.print(khoangcach);
  Serial.println(" cm");

  delay(23);
  }

int In_SenSor() {

  if(state_line==0)
  {ss5 =!digitalRead(Pin_ss5);
  ss2 = !digitalRead(Pin_ss2);
  ss3 = !digitalRead(Pin_ss3);
  ss4 = !digitalRead(Pin_ss4);
  ss1 = !digitalRead(Pin_ss1);}
  else if(state_line==1)
   {ss5 = digitalRead(Pin_ss5);
  ss2 = digitalRead(Pin_ss2);
  ss3 = digitalRead(Pin_ss3);
  ss4 = digitalRead(Pin_ss4);
  ss1 = digitalRead(Pin_ss1);}
  
  bitWrite(sensorValue, 0, ss5);
  bitWrite(sensorValue, 1, ss4);
  bitWrite(sensorValue, 2, ss3);
  bitWrite(sensorValue, 3, ss2);
  bitWrite(sensorValue, 4, ss1);

  return sensorValue;


}
 void forward(int speed)
 {
   analogWrite(motorA1, speed);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, speed);
   analogWrite(motorB2, 0);
 }

 void Backward(int speed)
 {
   analogWrite(motorA1, 0);
   analogWrite(motorA2, speed);
   analogWrite(motorB1, 0);
   analogWrite(motorB2, speed);
 }

 void Turn_right_180(int speed)
 {
   analogWrite(motorA1, speed);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, 0);
   analogWrite(motorB2, speed);
 }
 void Turn_right_90(int speed)
 {
   analogWrite(motorA1, speed);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, 0);
   analogWrite(motorB2, 0);
 }
   void Turn_right_45(int speed)
 {
   analogWrite(motorA1, speed);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, speed/3);
   analogWrite(motorB2, 0);
 }
    void Turn_right_30(int speed)
 {
   analogWrite(motorA1, speed);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, speed/2);
   analogWrite(motorB2, 0);
 }

 void Turn_left_180(int speed)
 {
   analogWrite(motorA1, 0);
   analogWrite(motorA2, speed);
   analogWrite(motorB1, speed);
   analogWrite(motorB2, 0);
 }
  void Turn_left_90(int speed)
 {
   analogWrite(motorA1, 0);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, speed);
   analogWrite(motorB2, 0);
 }
  void Turn_left_45(int speed)
 {
   analogWrite(motorA1, speed/3);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, speed);
   analogWrite(motorB2, 0);
 }
  void Turn_left_30(int speed)
 {
   analogWrite(motorA1, speed/2);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, speed);
   analogWrite(motorB2, 0);
 }
 void Robot_stop()
 {
   analogWrite(motorA1, 0);
   analogWrite(motorA2, 0);
   analogWrite(motorB1, 0);
   analogWrite(motorB2, 0);
 }
