
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <Wire.h>

#include "SevenSegmentManager.h"
#include "TrafficLevelManager.h"

// -----------------------------------------------------------------------------------------------------------------

// * I/O pin labeling

#define LIGHT_INTENSITY_SENSOR A0 // Potenciómetro que simula la intensidad de la luz en el ambiente para regular la iluminación de los dos semáforos
#define LDR2 A1 // Pendiente cómo usarlo...
#define CO2 A2  // Sensor de CO2

#define P1 37   // Botón que alterna entre modalidad de ciclo normal de semáforos y modalidad madrugada

// -----------------------------------------------------------------------------------------------------------------

// Semáforo 1 - Principal
#define S1LR 2  // Semáforo 1 - Luz Roja
#define S1LA 3  // Semáforo 1 - Luz Amarilla
#define S1LV 4  // Semáforo 1 - Luz Verde

#define S1V1 33 // Semáforo 1 - Sensor IR para medir volumen de tráfico menor
#define S1V2 34 // Semáforo 1 - Sensor IR para medir volumen de tráfico medio
#define S1V3 35 // Semáforo 1 - Sensor IR para medir volumen de tráfico alto

#define S1CSA 45
#define S1CSB 44
#define S1CSC 43
#define S1CSD 42
#define S1CSE 41
#define S1CSF 40
#define S1CSG 39

// -----------------------------------------------------------------------------------------------------------------

// Semáforo 2 - Secundario
#define S2LR 5  // Semáforo 2 - Luz Roja
#define S2LA 6  // Semáforo 2 - Luz Amarilla
#define S2LV 7  // Semáforo 2 - Luz Verde

#define S2V1 30 // Semáforo 2 - Sensor IR para medir volumen de tráfico menor
#define S2V2 31 // Semáforo 2 - Sensor IR para medir volumen de tráfico medio
#define S2V3 32 // Semáforo 2 - Sensor IR para medir volumen de tráfico alto

#define P2 36   // Botón para solicitar reducción de tiempo en verde, en el semáforo 2 para agilizar el paso peatonal

#define S2CSA 53
#define S2CSB 52
#define S2CSC 51
#define S2CSD 50
#define S2CSE 49
#define S2CSF 47
#define S2CSG 48

// -----------------------------------------------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------------------------------------------

// Library definitions
LiquidCrystal_I2C lcd(0x27, 20, 4);
SevenSegmentManager sevenSegmentManager1({S1CSA, S1CSB, S1CSC, S1CSD, S1CSE, S1CSF, S1CSG});
SevenSegmentManager sevenSegmentManager2({S2CSA, S2CSB, S2CSC, S2CSD, S2CSE, S2CSF, S2CSG});

// Definiciones de constantes para el sensor de CO2

// CO2
const float DC_GAIN = 8.5;                                                               // define the DC gain of amplifier CO2 sensor
const float ZERO_POINT_VOLTAGE = 0.265;                                                  // define the output of the sensor in volts when the concentration of CO2 is 400PPM
const float REACTION_VOLTAGE = 0.059;                                                    // define the “voltage drop” of the sensor when move the sensor from air into 1000ppm CO2
const float CO2Curve[3] = {2.602, ZERO_POINT_VOLTAGE, (REACTION_VOLTAGE / (2.602 - 3))}; // Line curve with 2 points

// Tiempos para cada estado (en milisegundos)
const unsigned long DURACION_ROJO       = 1000;
const unsigned long DURACION_NARANJA    = 2000;
const unsigned long DURACION_VERDE      = 3000;
const unsigned long DURACION_VERDE_T    = 2000;
const unsigned long DURACION_AMARILLO   = 1000;
const unsigned long DURACION_TITILACION = 250;

// Variable definitions
int state, previous_state;
float volts = 0; // Variable to store current voltage from CO2 sensor
float co2 = 0;   // Variable to store CO2 value

bool vP1       = false;
bool vP1Previo = false;
bool vP2       = false;

int ambientLightLevel = 255;
int currentBrightness = 1;
int vLDR2 = 0;
int vCO2 = 0;
float dCO2 = 0;

unsigned long previousMillis = 0;

void actualizarSemaforo(int pinRed, int pinYellow, int pinGreen, int redLevel, int yellowlevel, int greenLevel) {

    analogWrite(pinRed, redLevel * currentBrightness);
    analogWrite(pinYellow, yellowlevel * currentBrightness);
    analogWrite(pinGreen, greenLevel * currentBrightness);
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
    ambientLightLevel = analogRead(LIGHT_INTENSITY_SENSOR);

    currentBrightness = map(ambientLightLevel, 0, 1023, 32, 255);
    currentBrightness = ((currentBrightness - 32) / 32) * 32 + 32;
    currentBrightness = constrain(currentBrightness, 32, 255);

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

    vP1 = digitalRead(P1); // Toggle Modo Madrugada
    vP2 = digitalRead(P2); // Solicitud de peatón

    if (vP1 && !vP1Previo) {
        estaEnModalidadMadrugada = !estaEnModalidadMadrugada;
    }
    vP1Previo = vP1;

}

TrafficLevelManager trafficLevelManager1({S1V1, S1V2, S1V3});
TrafficLevelManager trafficLevelManager2({S2V1, S2V2, S2V3});

void setup()
{
    pinMode(P1, INPUT);
    pinMode(P2, INPUT);

    pinMode(S1LR, OUTPUT);
    pinMode(S1LA, OUTPUT);
    pinMode(S1LV, OUTPUT);

    pinMode(S2LR, OUTPUT);
    pinMode(S2LA, OUTPUT);
    pinMode(S2LV, OUTPUT);

    // Output cleaning
    digitalWrite(S1LR, LOW);
    digitalWrite(S1LA, LOW);
    digitalWrite(S1LV, LOW);

    digitalWrite(S2LR, LOW);
    digitalWrite(S2LA, LOW);
    digitalWrite(S2LV, LOW);

    // SevenSegments
    sevenSegmentManager1.init();
    sevenSegmentManager2.init();

    // * Communications
    Serial.begin(9600); // Start Serial communications with computer via Serial0 (TX0 RX0) at 9600 bauds
    lcd.init();
    lcd.backlight(); // Turn on LCD backlight
    state = ROJO_ROJO;
    previous_state = ROJO_AMARILLO;
}


void loop() {
    readAllData();
    trafficLevelManager1.updateTrafficLevels();
    trafficLevelManager2.updateTrafficLevels();

    unsigned long currentMillis = millis();

    // Verificar el estado del semáforo y realizar acciones
    switch (state) {
        case ROJO_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)true, (int)false, (int)false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, (int)false, (int)false); // Luz roja

            sevenSegmentManager1.print(-1);
            sevenSegmentManager2.print(-1);

            if (currentMillis - previousMillis >= DURACION_ROJO) {
                previousMillis = currentMillis;
                state = estaEnModalidadMadrugada ? AMARILLO_T_ROJO_T : NARANJA_ROJO;
            }
            trafficLevelManager1.calculateAdditionalTime();
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case NARANJA_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)true, (int)true, (int)false); // Luz roja y amarilla
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, (int)false, (int)false); // Luz roja
            sevenSegmentManager1.print(-1);
            sevenSegmentManager2.print(-1);

            if (currentMillis - previousMillis >= DURACION_NARANJA) {
                previousMillis = currentMillis;
                state = VERDE_ROJO;
            }
            trafficLevelManager1.calculateAdditionalTime();
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case VERDE_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)false, (int)false, (int)true);
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, (int)false, (int)false);

            sevenSegmentManager1.print(((trafficLevelManager1.getAdditionalTime() + DURACION_VERDE + DURACION_VERDE_T + DURACION_AMARILLO + 1000) - (currentMillis - previousMillis)) / 1000);
            sevenSegmentManager2.print(-1);

            if (currentMillis - previousMillis >= trafficLevelManager1.getAdditionalTime() + DURACION_VERDE) {
                previousMillis = currentMillis;
                state = VERDE_T_ROJO;
                trafficLevelManager1.reset();
            }
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case VERDE_T_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)false, (int)false, ((currentMillis - previousMillis) / DURACION_TITILACION) % 2);
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, (int)false, (int)false);
            sevenSegmentManager1.print(((DURACION_VERDE_T + DURACION_AMARILLO + 1000) - (currentMillis - previousMillis)) / 1000);
            sevenSegmentManager2.print(-1);

            if (currentMillis - previousMillis >= DURACION_VERDE_T) {
                previousMillis = currentMillis;
                state = AMARILLO_ROJO;
            }
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case AMARILLO_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)false, (int)true, (int)false);
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, (int)false, (int)false);
            sevenSegmentManager1.print(((DURACION_AMARILLO + 1000) - (currentMillis - previousMillis)) / 1000);
            sevenSegmentManager2.print(-1);

            if (currentMillis - previousMillis >= DURACION_AMARILLO) {
                sevenSegmentManager1.print(-1);
                previousMillis = currentMillis;
                state = ROJO_ROJO_V2;
            }
            trafficLevelManager1.calculateAdditionalTime();
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case ROJO_ROJO_V2:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)true, (int)false, (int)false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, (int)false, (int)false); // Luz roja
            sevenSegmentManager1.print(-1);
            sevenSegmentManager2.print(-1);

            if (currentMillis - previousMillis >= DURACION_ROJO) {
                previousMillis = currentMillis;
                state = estaEnModalidadMadrugada ? AMARILLO_T_ROJO_T : ROJO_NARANJA;
            }
            trafficLevelManager1.calculateAdditionalTime();
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case ROJO_NARANJA:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)true, (int)false, (int)false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, (int)true, (int)false); // Luz roja y amarilla
            sevenSegmentManager1.print(-1);
            sevenSegmentManager2.print(-1);

            if (currentMillis - previousMillis >= DURACION_NARANJA) {
                previousMillis = currentMillis;
                state = ROJO_VERDE;
            }
            trafficLevelManager1.calculateAdditionalTime();
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case ROJO_VERDE:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)true, (int)false, (int)false); // Luz roja
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)false, (int)false, (int)true); // Luz verde
            
            sevenSegmentManager1.print(-1);
            sevenSegmentManager2.print(((trafficLevelManager2.getAdditionalTime() + DURACION_VERDE + DURACION_VERDE_T + DURACION_AMARILLO + 1000) - (currentMillis - previousMillis)) / 1000);

            if (currentMillis - previousMillis >= trafficLevelManager2.getAdditionalTime() + DURACION_VERDE) {
                previousMillis = currentMillis;
                state = ROJO_VERDE_T;
                trafficLevelManager2.reset();
            }
            trafficLevelManager1.calculateAdditionalTime();
            break;

        case ROJO_VERDE_T:

            actualizarSemaforo(S1LR, S1LA, S1LV, (int)true, (int)false, (int)false);
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)false, (int)false, ((currentMillis - previousMillis) / DURACION_TITILACION) % 2); // Luz verde y amarilla (por ahora)

            sevenSegmentManager1.print(-1);
            sevenSegmentManager2.print(((DURACION_VERDE_T + DURACION_AMARILLO + 1000) - (currentMillis - previousMillis)) / 1000);

            if (currentMillis - previousMillis >= DURACION_VERDE_T) {
                previousMillis = currentMillis;
                state = ROJO_AMARILLO;
            }
            trafficLevelManager1.calculateAdditionalTime();
            break;

        case ROJO_AMARILLO:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)true, (int)false, (int)false);
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)false, (int)true, (int)false);
            sevenSegmentManager1.print(-1);
            sevenSegmentManager2.print(((DURACION_AMARILLO + 1000) - (currentMillis - previousMillis)) / 1000);

            if (currentMillis - previousMillis >= DURACION_AMARILLO) {
                previousMillis = currentMillis;
                state = ROJO_ROJO;
            }
            trafficLevelManager1.calculateAdditionalTime();
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case AMARILLO_T_ROJO_T:
            currentBrightness = 255;
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)false, ((currentMillis - previousMillis) / (2 * DURACION_TITILACION)) % 2, (int)false); // Luz amarilla titilando
            actualizarSemaforo(S2LR, S2LA, S2LV, ((currentMillis -(int) previousMillis) / (2 * DURACION_TITILACION)) % 2, (int)false, (int)false); // Luz roja titilando
            state = estaEnModalidadMadrugada ? AMARILLO_T_ROJO_T : ROJO_ROJO;
            break;
    }

    // Actualizar la pantalla LCD con el estado actual
    mostrarEstadoLCD();
}


