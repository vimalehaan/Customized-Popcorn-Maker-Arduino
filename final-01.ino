#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h> 
#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>  // Include the Watchdog Timer library

const int relaypump = 43;      //Pin number of relay which is used to pump motor
const int relayheater = 47;    //Pin number of relay which is used to heating element
const int tempPin = 23;       // Pin number of temperature sensor
const int irPin = 38;         // Pin number of IR sensor
const int gasSensor = A0;     // Pin number of Gas Sensor 
const int buzzerPin = 29;     // Pin number of Buzzer

const int fanin1 = 45;        // Pin number of exhauster fan

const int trigPin = 35;    // Pin number of Ultrasonic sensor used to measure height difference
const int echoPin = 37;

const int stepPin = 31;       // Pin number of stepper motor which is used to lift
const int dirPin = 33;

const int servocornPin = 24;
const int servochocPin = 22;
const int servosaltPin = 11;
const int servoendpin1 = 13;
const int servoendpin2 = 12;

OneWire oneWire(tempPin);         // setup a oneWire instance
DallasTemperature tempSensor(&oneWire); // pass oneWire to DallasTemperature library

float tempCelsius;    // temperature in Celsius
float tempFahrenheit; // temperature in Fahrenheit

Servo servoMcorn;  // create servo object to control a servo
Servo servoMchoc;
Servo servoMsalt;
Servo servoend1;
Servo servoend2;

int pos = 0;    // variable to store the servo position

bool startprocess=false;
bool vibr=true;
bool vibrlow=true;
bool timervalue=true;

const byte ROWS = 4; // Number of rows on the keypad
const byte COLS = 4; // Number of columns on the keypad
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6};    // row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3, 2};    // column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27,20,4); 

String flavorChoice = "";   // Stores the selected flavor
String sizeChoice = "";     // Stores the selected size
int packetChoice = 0;       // Stores the selected number of packets
bool continueProcess = true; // Flag to indicate if the process should continue

int flavor;
int size;

bool cup=0;
int delaycup;

bool gas=0;

int prevValue = HIGH; 

int price=0;
int finalprice=0;

float gasValue;  //variable to store sensor value
int heightx=1;
int prevx=1;
int movement=0;

unsigned long startTime = 0;
unsigned long targetTime = 1UL * 30UL * 1000UL; 

void setup() {
  lcd.init();      
  lcd.backlight();
  Serial.begin(9600); // initialize serial monitor
  tempSensor.begin();    // initialize the temperature sensor
  Serial.println("Tempreture sensor started");
  Serial.println("Gas sensor Starting up!");

  servoMcorn.attach(servocornPin); 
  servoMchoc.attach(servochocPin);
  servoMsalt.attach(servosaltPin);
  servoend1.attach(servoendpin1);
  servoend2.attach(servoendpin2);

  servoMcorn.write(100);
  servoMchoc.write(120);
  servoend1.write(95);
  servoend2.write(100);
  servoMsalt.write(45);
  pinMode(relaypump, OUTPUT);  
  pinMode(relayheater, OUTPUT);
  
  pinMode(fanin1, OUTPUT); 

  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT);
  
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);  

  digitalWrite(relaypump, LOW);
  digitalWrite(fanin1, LOW);
   
}

void loop() {
  
  corncheck();
  //To get input from a user
  displaymain();

  if(startprocess){  
  temprelay(size,flavor);}
  //To start stepper motor to lift
  stepperlift();
  
  //check cup is there or not if cup is present end servo motor will work 
  while(cupcheck(prevValue)==0){
  //To wait till user place a cup
  digitalWrite(buzzerPin, HIGH);
  delay(500);
  digitalWrite(buzzerPin, LOW);
  delay(500); 
  }
  delay(3000);
  servoEndopen(size);
  printmessage("Take your POPCORN");

  int takecup = digitalRead(irPin);
  while(takecup == LOW){
  takecup = digitalRead(irPin);
  }
  delay(3000);
  prevValue=HIGH;
  
  pricecalculator(flavor, size);
  resetChoices();
 }

void corncheck(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);

  long distance = duration * 0.0343 / 2;

  if(distance > 70){
    lcd.println("Please fill corn");
    delay(2000);
    lcd.clear();
  }
  delay(500);
}

int displaymain(){
    if (continueProcess) {
    if (flavorChoice == "") {
      flavor = selectFlavor();
      }
    if (sizeChoice == "") {
      size = selectSize();
    }
    resetask();
  }
}

int selectFlavor() {

  lcd.setCursor(0, 0);
  lcd.print("Select Flavor:");
  lcd.setCursor(0, 1);
  lcd.print("A - salt & turmeric");
  lcd.setCursor(0, 2);
  lcd.print("B - Chocolate");
  lcd.setCursor(13, 3);
  lcd.print("D-RESET");
  
  char key=keypad.waitForKey();
  int flavorReturn;  
  if (key != NO_KEY) {
       switch (key) {
      case 'A':
        flavorChoice = "Salt & Turmeric";
        flavorReturn=1;
        break;
      case 'B':
        flavorChoice = "Chocolate";
        flavorReturn=2;
        break;
      case 'D':
        resetChoices();
        break;
      default:
        displayErrorMessage("Invalid Flavor");
        break;
    }
      if (flavorChoice != "") {
      displaySelectedChoice(flavorChoice);
    }
    return flavorReturn;
  }
  }

int selectSize() {

  int sizereturn;  
  lcd.setCursor(0, 0);
  lcd.print("Select size:");
  lcd.setCursor(0, 2);
  lcd.print("2 - Large");
  lcd.setCursor(0, 1);
  lcd.print("1 - Small");
  lcd.setCursor(13, 3);
  lcd.print("D-RESET");

  char key=keypad.waitForKey();
  if (key != NO_KEY) {
    switch (key) {
      case '1':
        sizeChoice = "Small";
        sizereturn=1;
        break;
      case '2':
        sizeChoice = "Large";
        sizereturn=2;
        break;
      case 'D':
        resetChoices();
        break;
      default:
        displayErrorMessage("Invalid Size");
        break;
    }

    if (sizeChoice != "") {
      displaySelectedChoice(sizeChoice);
    }
  }
  return sizereturn;
}

void displayErrorMessage(const char* message) {
  lcd.clear();
  lcd.setCursor(3,1);
  lcd.print(message);
  delay(1000);
  lcd.clear();
  restartProgram();
}

void displaySelectedChoice(const String& choice) {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(choice);
  lcd.setCursor(0, 2);
  lcd.print("Selected!");
  delay(2000);
  lcd.clear();
}

void resetask(){
  lcd.setCursor(0, 0);
  lcd.print("Press");
  lcd.setCursor(5, 1);
  lcd.print("A - Start");
  lcd.setCursor(5, 2);
  lcd.print("B - Reset"); 

  char key=keypad.waitForKey();
     if (key != NO_KEY) {
        if (key == 'B') {
        lcd.clear();
        resetChoices();
        }
        if(key == 'A'){
        lcd.setCursor(0,3);
        lcd.print("Process start");
        delay(3000);
        lcd.clear();
        startprocess=true;        
        continueProcess = false;
       }
  }
}

void resetChoices() {
  flavorChoice = "";
  sizeChoice = "";
  packetChoice = 0;
  price="";
  finalprice="";
  continueProcess = true;
  lcd.clear();
  lcd.setCursor(3,1);
  lcd.print("Choices Reset");
  delay(1000);
  lcd.clear();
  restartProgram();
}

void restartProgram() {
  wdt_disable();  // Disable the Watchdog Timer
  wdt_enable(WDTO_15MS);  // Enable the Watchdog Timer with a short timeout
  while (1) {}  // Enter an infinite loop to trigger a watchdog reset
}
//--------------------------------------------------------------------------------------------above display

//Heater and ingrediant flow control system-----------------------------------------------------------------
void temprelay(int size, int flavor){
  if (size == 1) 
    {targetTime= 3UL * 60UL * 1000UL + 10UL * 1000UL;}
  else if (size == 2) 
    {targetTime= 3UL * 60UL * 1000UL + 50UL * 1000UL;}
  else {
    targetTime = 0;
  }

  tempSensor.requestTemperatures();             // send the command to get temperatures
  tempCelsius = tempSensor.getTempCByIndex(0);  // read temperature in Celsius

  Serial.println("Temperature: ");
  Serial.print(tempCelsius);
  Serial.println("°C");
  delay(500);
  
  digitalWrite(relayheater, HIGH);
  Serial.println("Heater relay started");

  while(tempCelsius<40){
  digitalWrite(relayheater, HIGH);
  tempSensor.requestTemperatures();        
  tempCelsius = tempSensor.getTempCByIndex(0);
  Serial.println(tempCelsius);    
  }

  if(tempCelsius>=50){
  ingrediantheat(size,flavor);}
  else if(tempCelsius>=40){
  ingrediant(size,flavor);}
  
  timervalue=timer(targetTime); 
    while(timervalue){
    timervalue=timer(targetTime); 
     
    tempSensor.requestTemperatures();             
    tempCelsius = tempSensor.getTempCByIndex(0);

    if(tempCelsius < 83){
    digitalWrite(relayheater, HIGH);
    delay(3000);
    Serial.println("heater restarted");
    }
    if(tempCelsius>120){
    digitalWrite(relayheater, LOW);
    Serial.println("Heater overheating");

    }
    else if(tempCelsius>90){
    digitalWrite(relayheater, LOW);
    delay(5000);
    Serial.println("Heater limit reached Heater relay off");
    digitalWrite(relayheater, HIGH);
    delay(7000);
    }
    Serial.println(tempCelsius);    

    gas=gasdetect();
    if(gas == true){
    turnonFan();
    }
    }
  
  digitalWrite(relayheater, LOW);
  Serial.println("Heater stoped");
  turnoffFan();
}

//works like a timer 
bool timer(unsigned long targetTime) {
  if (startTime == 0) {
    Serial.println("Timer started");
    // Start the timer on the first call
    startTime = millis();
    return true;
  } else {
    unsigned long currentTime = millis();
    if (currentTime - startTime >= targetTime) {
      startTime = 0;
      return false;
    } else {
      return true;
    }
  }
}

void ingrediant(int size,int flavor){
  Serial.println("Ingrediant flow process started");
  pumprelay(size);
  delay(30000);
  if(flavor == 2){
    servoChoc();
    delay(30000);
  }
  servocorn(size);
  if(flavor == 1){
    delay(2000);
  servoSalt(size);
  }
}

void ingrediantheat(int size,int flavor){
  Serial.println("Ingrediant flow process started");
  delay(60000);
  pumprelay(size);
  delay(30000);
  if(flavor == 2){
    servoChoc();
    delay(30000);
  }
  servocorn(size);
  if(flavor == 1){
    delay(2000);
  servoSalt(size);
  }
}

void pumprelay(int size){
  int pumpdelay;
    if (size == 1) 
   {pumpdelay = 900;}
    else if (size == 2) 
    {pumpdelay = 1100;}
  else {
    targetTime = 0;
  }

  digitalWrite(relaypump, HIGH); 
  delay(pumpdelay);
  digitalWrite(relaypump, LOW);
  delay(1000);                   
  Serial.println("Pump motor working");
}

void servocorn(int size) {
  int delayTime;

  if (size == 1) {
    delayTime = 8;  
  } else if (size == 2) {
    delayTime = 10;  
  } else {
    delayTime = 0;
  }

  for (pos = 0; pos <= 100; pos += 1) {
    servoMcorn.write(pos);
    delay(delayTime);                          
  }
  servoMcorn.write(100);
  delay(500);
  
  Serial.println("Corn flow ok");
}

void servoChoc()
  {
  for(int i=1; i<=3; i++)
  {servoMchoc.write(20);
  delay(1000);
  servoMchoc.write(120);
  delay(1000);}
  
  Serial.println("Chocolate flavor flow start");
  }

void servoSalt(int size){
  int countsalt;
  if(size==1){countsalt=3;}
  if(size==2){countsalt=4;}

  for(int i=1; i<=countsalt; i++){
  servoMsalt.write(140);
  delay(1000);
  servoMsalt.write(45);
  delay(1000);}
  

  Serial.println("Pepper and salt flavor flow start");
}
//-----------------------------------------------------------------------------------------------------

//---------------------------------Stepper motor lift--------------------------------------------------
void stepperlift(){
  rotateMotorClockwise(600);
  delay(500);

  for(int a=1; a<=5; a++)
{  rotateMotorCounterclockwise(200);
  delay(200);
  rotateMotorClockwise(200);
  delay(300);
  rotateMotorCounterclockwise(200);
  delay(200);
  rotateMotorClockwise(200);
  delay(300);}

  rotateMotorCounterclockwise(600);
  delay(5000);

  }

void rotateMotorClockwise(int time) {
  digitalWrite(dirPin, HIGH); // Set the direction to clockwise

  for(int x = 0; x < time; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }
  // Stop the motor
  digitalWrite(dirPin, LOW);
  digitalWrite(stepPin, LOW);
}

void rotateMotorCounterclockwise(int time) {
  digitalWrite(dirPin, LOW); // Set the direction to counterclockwise

  for(int x = 0; x < time; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }
  // Stop the motor
  digitalWrite(dirPin, LOW);
  digitalWrite(stepPin, LOW);
}

//-----------------------------------------------------------------------------------------------------

//-------------------------------Cup checking----------------------------------------------------------

bool cupcheck(bool prevValue){
	int SensorValue = digitalRead(irPin);

	if (SensorValue == LOW && prevValue == HIGH)
	{ lcd.clear();
		lcd.setCursor(1, 1);
		lcd.print("POPCORN filling up");
    lcd.setCursor(1, 2);
		lcd.print("Please wait");       
    Serial.println("Cup OK");
    cup=1;
	}
	else if (SensorValue == HIGH)
	{ lcd.setCursor(1, 1);
		lcd.print("POPCORN ready!");
    lcd.setCursor(1, 2);
		lcd.print("Insert a cup");    
    Serial.println("NO cup");
    cup=0;
	}

	prevValue = SensorValue; // Update the previous sensor value
  return cup;
}
//-------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
void servoEndopen(int size){
  if (size == 2) {
    delaycup = 1000;
  } else if (size == 1) {
    delaycup = 800;
  } else {
    delaycup = 0;}
  servoend1.write(95);
  servoend2.write(100);
  delay(1000);
  
  servoend1.write(0);
  servoend2.write(20);
  delay(delaycup);

  servoend1.write(95);
  servoend2.write(100);
  delay(1000);
  }
//----------------------------------------------------------------------------------------
//Gas detection
bool gasdetect(){
  gasValue = analogRead(gasSensor); // read analog input pin 0
  
  Serial.print("Sensor Value: ");
  Serial.print(gasValue);
  
  if(gasValue > 35)
  {
    Serial.print("Smoke detected!");
    return 1;
  }
  
  Serial.println("");
  delay(2000); // wait 2s for next reading
  return 0;
}

//----------------------------------------------------------------------------------------
//To On fan
void turnonFan(){              
  digitalWrite(fanin1, HIGH);
  delay(3000);
  digitalWrite(fanin1, LOW);
  delay(1000);
}

//To off fan
void turnoffFan(){
  digitalWrite(fanin1, LOW);
}
//-----------------------------------------------------------------------------------------
void pricecalculator(int flavor,int size){
  if(flavor==1){
    if(size==1){price=100;}
    if(size==2){price=150;}
  }
    if(flavor==2){
    if(size==1){price=150;}
    if(size==2){price=250;}
  }
  lcd.clear();
  lcd.print("Total amount:Rs.");
  lcd.print(price);
  delay(7000);
  lcd.clear();
}
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
void printmessage(const String& message){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}