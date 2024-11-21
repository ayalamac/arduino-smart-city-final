
#include <Wire.h>              //Library required for I2C comms (LCD)
#include <LiquidCrystal_I2C.h> //Library for LCD display via I2C
#include <math.h>              //Mathematics library for pow function (CO2 computation)

// I/O pin labeling

#define NIVEL_LUZ_AMBIENTE A0 // Potenciómetro que simula la intensidad de la luz en el ambiente para regular la iluminación de los dos semáforos
#define LDR2 A1 // Pendiente cómo usarlo...
#define CO2 A2  // Sensor de CO2

#define P1 37   // Botón que alterna entre modalidad de ciclo normal de semáforos y modalidad madrugada

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// Semáforo 1 - Principal
#define S1LR 22  // Semáforo 1 - Luz Roja
#define S1LA 23  // Semáforo 1 - Luz Amarilla
#define S1LV 24  // Semáforo 1 - Luz Verde

#define S1V1 33 // Semáforo 1 - Sensor IR para medir volumen de tráfico menor
#define S1V2 34 // Semáforo 1 - Sensor IR para medir volumen de tráfico medio
#define S1V3 35 // Semáforo 1 - Sensor IR para medir volumen de tráfico alto

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// Semáforo 2 - Secundario
#define S2LR 25  // Semáforo 2 - Luz Roja
#define S2LA 26  // Semáforo 2 - Luz Amarilla
#define S2LV 27  // Semáforo 2 - Luz Verde

#define S2V1 30 // Semáforo 2 - Sensor IR para medir volumen de tráfico menor
#define S2V2 31 // Semáforo 2 - Sensor IR para medir volumen de tráfico medio
#define S2V3 32 // Semáforo 2 - Sensor IR para medir volumen de tráfico alto

#define P2 36   // Botón para solicitar reducción de tiempo en verde, en el semáforo 2 para agilizar el paso peatonal

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// Máquina de estados
#define ROJO_ROJO 0
#define ROJO_NARANJA 1
#define ROJO_VERDE 2
#define ROJO_VERDE_T 3
#define ROJO_AMARILLO 4

#define ROJO_ROJO_V2 5
#define NARANJA_ROJO 6
#define VERDE_ROJO 7
#define VERDE_T_ROJO 8
#define AMARILLO_ROJO 9

// Estado nocturno
#define AMARILLO_T_ROJO_T 10

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// Library definitions
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Definiciones de constantes para el sensor de CO2

// CO2
const float DC_GAIN = 8.5;                                                               // define the DC gain of amplifier CO2 sensor
const float ZERO_POINT_VOLTAGE = 0.265;                                                  // define the output of the sensor in volts when the concentration of CO2 is 400PPM
const float REACTION_VOLTAGE = 0.059;                                                    // define the “voltage drop” of the sensor when move the sensor from air into 1000ppm CO2
const float CO2Curve[3] = {2.602, ZERO_POINT_VOLTAGE, (REACTION_VOLTAGE / (2.602 - 3))}; // Line curve with 2 points

// Tiempos para cada estado (en milisegundos)
const unsigned long DURACION_ROJO       = 4000;
const unsigned long DURACION_NARANJA    = 2000;
const unsigned long DURACION_VERDE      = 4000;
const unsigned long DURACION_VERDE_T    = 2400;
const unsigned long DURACION_AMARILLO   = 2400;
const unsigned long DURACION_TITILACION = 300;

// Variable definitions
int state, previous_state;
float volts = 0; // Variable to store current voltage from CO2 sensor
float co2 = 0;   // Variable to store CO2 value

bool vP1 = true;
bool vP2 = false;
bool vS1V3 = false;
bool vS1V2 = false;
bool vS1V1 = false;
bool vS2V3 = false;
bool vS2V2 = false;
bool vS2V1 = false;
int vNIVEL_LUZ_AMBIENTE = 0;
int vLDR2 = 0;
int vCO2 = 0;
float dCO2 = 0;

unsigned long previousMillis = 0;

void actualizarSemaforo(int pinRed, int pinYellow, int pinGreen, bool red, bool yellow, bool green) {
    digitalWrite(pinRed, red);
    digitalWrite(pinYellow, yellow);
    digitalWrite(pinGreen, green);
}

void mostrarEstadoLCD() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Estado:");
    switch (state) {
        case ROJO_ROJO:
            lcd.print(" ROJO");
            break;
        case ROJO_AMARILLO:
        case AMARILLO_ROJO:
            lcd.print(" AMARILLO");
            break;
        case ROJO_VERDE:
            lcd.print(" VERDE");
            break;
    }
    lcd.setCursor(0, 1);
    lcd.print("Tiempo:");
    lcd.print((previousMillis + ((state == ROJO_ROJO || state == ROJO_VERDE) ? DURACION_ROJO : DURACION_AMARILLO)) - millis());
}

bool estaEnModalidadMadrugada = false;

void readAllData()
{
    vNIVEL_LUZ_AMBIENTE = analogRead(NIVEL_LUZ_AMBIENTE);
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
    vS1V3 = digitalRead(S1V3);
    vS1V2 = digitalRead(S1V2);
    vS1V1 = digitalRead(S1V1);
    vS2V3 = digitalRead(S2V3);
    vS2V2 = digitalRead(S2V2);
    vS2V1 = digitalRead(S2V1);

    if (vP1)
    {
        digitalWrite(S1LR, 0);
        digitalWrite(S1LA, 0);
        digitalWrite(S1LV, 0);
        estaEnModalidadMadrugada = !estaEnModalidadMadrugada;
    }
    else
    {
        digitalWrite(S1LR, vS1V1);
        digitalWrite(S1LA, vS1V2);
        digitalWrite(S1LV, vS1V3);
    }
    if (vP2)
    {
        digitalWrite(S2LR, 0);
        digitalWrite(S2LA, 0);
        digitalWrite(S2LV, 0);
    }
    else
    {
        digitalWrite(S2LR, vS2V1);
        digitalWrite(S2LA, vS2V2);
        digitalWrite(S2LV, vS2V3);
    }
}



void setup()
{
    // Input pin config
    pinMode(P1, INPUT);   // Traffic light 1 button as Input
    pinMode(P2, INPUT);   // Traffic light 2 button as Input
    pinMode(S1V3, INPUT); // Infrared sensor 1 in traffic light 1 as Input
    pinMode(S1V2, INPUT); // Infrared sensor 2 in traffic light 1 as Input
    pinMode(S1V1, INPUT); // Infrared sensor 3 in traffic light 1 as Input
    pinMode(S2V3, INPUT); // Infrared sensor 4 in traffic light 2 as Input
    pinMode(S2V2, INPUT); // Infrared sensor 5 in traffic light 2 as Input
    pinMode(S2V1, INPUT); // Infrared sensor 6 in traffic light 2 as Input

    // Output pin config
    pinMode(S1LR, OUTPUT); // Red traffic light 1 as Output
    pinMode(S1LA, OUTPUT); // Yellow traffic light 1 as Output
    pinMode(S1LV, OUTPUT); // Green traffic light 1 as Output
    pinMode(S2LR, OUTPUT); // Red traffic light 2 as Output
    pinMode(S2LA, OUTPUT); // Yellow traffic light 2 as Output
    pinMode(S2LV, OUTPUT); // Green traffic light 2 as Output

    // Output cleaning
    digitalWrite(S1LR, LOW); // Turn Off Red traffic light 1
    digitalWrite(S1LA, LOW); // Turn Off Yellow traffic light 1
    digitalWrite(S1LV, LOW); // Turn Off Green traffic light 1
    digitalWrite(S2LR, LOW); // Turn Off Red traffic light 2
    digitalWrite(S2LA, LOW); // Turn Off Yellow traffic light 2
    digitalWrite(S2LV, LOW); // Turn Off Green traffic light 2

    // Communications
    Serial.begin(9600); // Start Serial communications with computer via Serial0 (TX0 RX0) at 9600 bauds
    lcd.init();
    lcd.backlight(); // Turn on LCD backlight
    state = ROJO_ROJO;
    previous_state = ROJO_AMARILLO;
}

void loop() {
    unsigned long currentMillis = millis();

    // Verificar el estado del semáforo y realizar acciones
    switch (state) {
        case ROJO_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, true, false, false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, true, false, false); // Luz roja
            if (currentMillis - previousMillis >= DURACION_ROJO) {
                previousMillis = currentMillis;
                state = !estaEnModalidadMadrugada ? NARANJA_ROJO : AMARILLO_T_ROJO_T;
            }
            break;
            
        case NARANJA_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, true, true, false); // Luz roja y amarilla
            actualizarSemaforo(S2LR, S2LA, S2LV, true, false, false); // Luz roja
            if (currentMillis - previousMillis >= DURACION_NARANJA) {
                previousMillis = currentMillis;
                state = VERDE_ROJO;
            }
            break;

        case VERDE_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, false, false, true); // Luz verde
            actualizarSemaforo(S2LR, S2LA, S2LV, true, false, false); // Luz roja
            if (currentMillis - previousMillis >= DURACION_VERDE) {
                previousMillis = currentMillis;
                state = VERDE_T_ROJO;
            }
            break;

        case VERDE_T_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, false, false, ((currentMillis - previousMillis) / DURACION_TITILACION) % 2);
            actualizarSemaforo(S2LR, S2LA, S2LV, true, false, false);

            if (currentMillis - previousMillis >= DURACION_VERDE_T) {
                previousMillis = currentMillis;
                state = AMARILLO_ROJO;
            }
            break;

        case AMARILLO_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, false, true, false); // Luz amarilla
            actualizarSemaforo(S2LR, S2LA, S2LV, true, false, false); // Luz roja
            if (currentMillis - previousMillis >= DURACION_AMARILLO) {
                previousMillis = currentMillis;
                state = ROJO_ROJO_V2;
            }
            break;

        case ROJO_ROJO_V2:
            actualizarSemaforo(S1LR, S1LA, S1LV, true, false, false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, true, false, false); // Luz roja
            if (currentMillis - previousMillis >= DURACION_ROJO) {
                previousMillis = currentMillis;
                state = !estaEnModalidadMadrugada ? ROJO_NARANJA : AMARILLO_T_ROJO_T;
            }
            break;

        case ROJO_NARANJA:
            actualizarSemaforo(S1LR, S1LA, S1LV, true, false, false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, true, true, false); // Luz roja y amarilla
            if (currentMillis - previousMillis >= DURACION_NARANJA) {
                previousMillis = currentMillis;
                state = ROJO_VERDE;
            }
            break;

        case ROJO_VERDE:
            actualizarSemaforo(S1LR, S1LA, S1LV, true, false, false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, false, false, true); // Luz verde
            if (currentMillis - previousMillis >= DURACION_VERDE) {
                previousMillis = currentMillis;
                state = ROJO_VERDE_T;
            }
            break;

        case ROJO_VERDE_T:

            actualizarSemaforo(S1LR, S1LA, S1LV, true, false, false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, false, false, ((currentMillis - previousMillis) / DURACION_TITILACION) % 2); // Luz verde y amarilla (por ahora)

            if (currentMillis - previousMillis >= DURACION_VERDE_T) {
                previousMillis = currentMillis;
                state = ROJO_AMARILLO;
            }
            break;

        case ROJO_AMARILLO:
            actualizarSemaforo(S1LR, S1LA, S1LV, true, false, false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, false, true, false); // Luz verde y amarilla (por ahora)
            if (currentMillis - previousMillis >= DURACION_AMARILLO) {
                previousMillis = currentMillis;
                state = ROJO_ROJO;
            }
            break;

        case AMARILLO_T_ROJO_T:
            actualizarSemaforo(S1LR, S1LA, S1LV, false, ((currentMillis - previousMillis) / DURACION_TITILACION) % 2, false); // Luz amarilla titilando
            actualizarSemaforo(S2LR, S2LA, S2LV, ((currentMillis - previousMillis) / DURACION_TITILACION) % 2, false, false); // Luz roja titilando
            state = estaEnModalidadMadrugada ? AMARILLO_T_ROJO_T : ROJO_ROJO;
            break;
    }

    // Actualizar la pantalla LCD con el estado actual
    mostrarEstadoLCD();
}

