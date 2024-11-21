// Pruebas de los perifericos del smart city
// Library definition
#include <Wire.h>              //Library required for I2C comms (LCD)
#include <LiquidCrystal_I2C.h> //Library for LCD display via I2C
#include <math.h>              //Mathematics library for pow function (CO2 computation)

// I/O pin labeling
#define LDR1 A0 // LDR Light sensor from traffic light 1 connected in pin A0
#define LDR2 A1 // LDR Light sensor from traffic light 2 connected in pin A1
#define CO2 A2  // CO2 sensor connected in pin A3
#define P1 37   // Traffic light 1 button connected in pin 37
#define P2 36   // Traffic light 2 button connected in pin 36
#define CNY1 35 // Infrared sensor 1 in traffic light 1 connected in pin 35
#define CNY2 34 // Infrared sensor 2 in traffic light 1 connected in pin 34
#define CNY3 33 // Infrared sensor 3 in traffic light 1 connected in pin 33
#define CNY4 32 // Infrared sensor 4 in traffic light 2 connected in pin 32
#define CNY5 31 // Infrared sensor 5 in traffic light 2 connected in pin 31
#define CNY6 30 // Infrared sensor 6 in traffic light 2 connected in pin 30
#define LR1 22  // Red traffic light 1 connected in pin 22
#define LY1 23  // Yellow traffic light 1 connected in pin 23
#define LG1 24  // Green traffic light 1 connected in pin 24
#define LR2 25  // Red traffic light 2 connected in pin 25
#define LY2 26  // Yellow traffic light 2 connected in pin 26
#define LG2 27  // Green traffic light 2 connected in pin 27

// Constant definitions
//->CO2
const float DC_GAIN = 8.5;                                                               // define the DC gain of amplifier CO2 sensor
const float ZERO_POINT_VOLTAGE = 0.265;                                                  // define the output of the sensor in volts when the concentration of CO2 is 400PPM
const float REACTION_VOLTAGE = 0.059;                                                    // define the “voltage drop” of the sensor when move the sensor from air into 1000ppm CO2
const float CO2Curve[3] = {2.602, ZERO_POINT_VOLTAGE, (REACTION_VOLTAGE / (2.602 - 3))}; // Line curve with 2 points

// Variable definitions
float volts = 0;  // Variable to store current voltage from CO2 sensor
float co2 = 0;    // Variable to store CO2 value

// Library definitions
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Variables
bool vP1 = false;
bool vP2 = false;
bool vCNY1 = false;
bool vCNY2 = false;
bool vCNY3 = false;
bool vCNY4 = false;
bool vCNY5 = false;
bool vCNY6 = false;
int vLDR1 = 0;
int vLDR2 = 0;
int vCO2 = 0;
float dCO2 = 0;

void setup()
{
  // Input pin config
  pinMode(P1, INPUT);   // Traffic light 1 button as Input
  pinMode(P2, INPUT);   // Traffic light 2 button as Input
  pinMode(CNY1, INPUT); // Infrared sensor 1 in traffic light 1 as Input
  pinMode(CNY2, INPUT); // Infrared sensor 2 in traffic light 1 as Input
  pinMode(CNY3, INPUT); // Infrared sensor 3 in traffic light 1 as Input
  pinMode(CNY4, INPUT); // Infrared sensor 4 in traffic light 2 as Input
  pinMode(CNY5, INPUT); // Infrared sensor 5 in traffic light 2 as Input
  pinMode(CNY6, INPUT); // Infrared sensor 6 in traffic light 2 as Input

  // Output pin config
  pinMode(LR1, OUTPUT); // Red traffic light 1 as Output
  pinMode(LY1, OUTPUT); // Yellow traffic light 1 as Output
  pinMode(LG1, OUTPUT); // Green traffic light 1 as Output
  pinMode(LR2, OUTPUT); // Red traffic light 2 as Output
  pinMode(LY2, OUTPUT); // Yellow traffic light 2 as Output
  pinMode(LG2, OUTPUT); // Green traffic light 2 as Output

  // Output cleaning
  digitalWrite(LR1, LOW); // Turn Off Red traffic light 1
  digitalWrite(LY1, LOW); // Turn Off Yellow traffic light 1
  digitalWrite(LG1, LOW); // Turn Off Green traffic light 1
  digitalWrite(LR2, LOW); // Turn Off Red traffic light 2
  digitalWrite(LY2, LOW); // Turn Off Yellow traffic light 2
  digitalWrite(LG2, LOW); // Turn Off Green traffic light 2

  // Communications
  Serial.begin(9600); // Start Serial communications with computer via Serial0 (TX0 RX0) at 9600 bauds
  lcd.init();
  lcd.backlight(); // Turn on LCD backlight
}

long lastime = 0;
void loop()
{
  readAllData();
  if (millis() - lastime > 1000) {
    lastime = millis();
    // lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LDR1       CNY 1 2 3");
    lcd.setCursor(0, 1);
    lcd.print("LDR2               ");
    lcd.setCursor(0, 2);
    lcd.print("CO2        CNY 4 5 6");
    lcd.setCursor(0, 3);
    lcd.print("P1   P2            ");
    lcd.setCursor(5, 0); lcd.print(vLDR1);
    lcd.setCursor(5, 1); lcd.print(vLDR2);
    lcd.setCursor(5, 2); lcd.print(vCO2);
    lcd.setCursor(3, 3); lcd.print(1 * vP1);
    lcd.setCursor(8, 3); lcd.print(1 * vP2);
    lcd.setCursor(15, 1); lcd.print(1 * vCNY1);
    lcd.setCursor(17, 1); lcd.print(1 * vCNY2);
    lcd.setCursor(19, 1); lcd.print(1 * vCNY3);
    lcd.setCursor(15, 3); lcd.print(1 * vCNY4);
    lcd.setCursor(17, 3); lcd.print(1 * vCNY5);
    lcd.setCursor(19, 3); lcd.print(1 * vCNY6);
  }
}

// Subroutines and functions
void readAllData()
{

  vLDR1 = analogRead(LDR1);
  vLDR2 = analogRead(LDR2);
  vCO2 = analogRead(CO2);

  dCO2 = 0;
  volts = analogRead(CO2) * 5.0 / 1023.0;
  if (volts / DC_GAIN >= ZERO_POINT_VOLTAGE)
  {
    dCO2 = -1;
  }
  else
  {
    dCO2 = pow(10, ((volts / DC_GAIN) - CO2Curve[1]) / CO2Curve[2] + CO2Curve[0]);
  }

  vP1 = digitalRead(P1);
  vP2 = digitalRead(P2);
  vCNY1 = digitalRead(CNY1);
  vCNY2 = digitalRead(CNY2);
  vCNY3 = digitalRead(CNY3);
  vCNY4 = digitalRead(CNY4);
  vCNY5 = digitalRead(CNY5);
  vCNY6 = digitalRead(CNY6);

  if (vP1) {
    digitalWrite(LR1, 0);
    digitalWrite(LY1, 0);
    digitalWrite(LG1, 0);
  } else {
    digitalWrite(LR1, vCNY3);
    digitalWrite(LY1, vCNY2);
    digitalWrite(LG1, vCNY1);
  }
  if (vP2) {
    digitalWrite(LR2, 0);
    digitalWrite(LY2, 0);
    digitalWrite(LG2, 0);
  } else {
    digitalWrite(LR2, vCNY6);
    digitalWrite(LY2, vCNY5);
    digitalWrite(LG2, vCNY4);
  }
}