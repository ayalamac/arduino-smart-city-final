
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <Wire.h>

#include "SevenSegmentManager.h"
#include "TrafficLevelManager.h"

// -----------------------------------------------------------------------------------------------------------------

// * I/O pin labeling

#define LIGHT_INTENSITY_SENSOR A0      // Potenciómetro que simula la intensidad de la luz en el ambiente para regular la iluminación de los dos semáforos
#define AMBULANCE_PROXIMITY_SENSOR A1  // Potenciómetro que simula la proximidad de una ambulancia para darle prioridad en el paso
#define CO2_SENSOR A2                         // Potenciómetro que simula la concentración de CO2 en el ambiente

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
#define ROJO_ROJO 100
#define ROJO_NARANJA 200
#define ROJO_VERDE 300
#define ROJO_VERDE_T 400
#define ROJO_AMARILLO 500

#define ROJO_ROJO_V2 600
#define NARANJA_ROJO 700
#define VERDE_ROJO 800
#define VERDE_T_ROJO 900
#define AMARILLO_ROJO 1000

// Estado nocturno
#define AMARILLO_T_ROJO_T 1100
#define PASO_AMBULANCIA_VIA1 1200

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
unsigned long duracionVerde      = 3000;
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
bool vP2Previo = false;

int ambientLightLevel = 255;
int currentBrightness = 1;
int ambulanceRelativePosition = 0;
int vCO2 = 0;
float dCO2 = 0;

unsigned long previousMillis = 0;

void actualizarSemaforo(int pinRed, int pinYellow, int pinGreen, int redLevel, int yellowlevel, int greenLevel) {

    analogWrite(pinRed, redLevel * currentBrightness);
    analogWrite(pinYellow, yellowlevel * currentBrightness);
    analogWrite(pinGreen, greenLevel * currentBrightness);
}

// void mostrarEstadoLCD() {
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Estado:");
//     switch (state) {
//         case ROJO_ROJO:
//             lcd.print(" ROJO");
//             break;
//         case ROJO_AMARILLO:
//         case AMARILLO_ROJO:
//             lcd.print(" AMARILLO");
//             break;
//         case ROJO_VERDE:
//             lcd.print(" VERDE");
//             break;
//     }
//     lcd.setCursor(0, 1);
//     lcd.print("Tiempo:");
//     lcd.print((previousMillis + ((state == ROJO_ROJO || state == ROJO_VERDE) ? DURACION_ROJO : DURACION_AMARILLO)) - millis());
// }

bool estaEnModalidadMadrugada = false;
bool botonPeatonalsolicitado = false;

void readAllData()
{
    ambientLightLevel = analogRead(LIGHT_INTENSITY_SENSOR);
    ambulanceRelativePosition = analogRead(AMBULANCE_PROXIMITY_SENSOR);

    vCO2 = analogRead(CO2_SENSOR);

    dCO2 = 0;
    volts = analogRead(CO2_SENSOR) * 5.0 / 1023.0;
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

    botonPeatonalsolicitado = false;

    if (vP2 && !vP2Previo) {
        // Reducir tiempo de verde en semáforo 2
        if (state == ROJO_VERDE && !botonPeatonalsolicitado ) {
            duracionVerde = duracionVerde - 2000;
            botonPeatonalsolicitado = true;
        }
    }
    vP2Previo = vP2;

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

void calculateBrightnessLevel() {
    currentBrightness = map(ambientLightLevel, 0, 1023, 32, 255);
    currentBrightness = ((currentBrightness - 32) / 32) * 32 + 32;
    currentBrightness = constrain(currentBrightness, 32, 255);
}

bool isAmbulancePassingThrough() {
    int proximityRate = (trafficLevelManager1.getAdditionalTime() + 1000) / 1000;
    int requiredProximity = 128 * proximityRate;
    int startThreshold = 511 - requiredProximity;
    int endThreshold = 511 + requiredProximity;
    return ambulanceRelativePosition >= startThreshold && ambulanceRelativePosition <= endThreshold;
}

long lastime = 0;
String modoUso = "Ciclo Normal   ";
void loop() {
    readAllData();

    if (millis() - lastime > 1000) {

        Serial.println(String(vCO2)); // Para visualizar en el monitor con Ubidots

        if (botonPeatonalsolicitado) { modoUso = "Paso Peatonal solicitado"; }
        if (estaEnModalidadMadrugada) { modoUso = "Madrugada      "; }
        if (isAmbulancePassingThrough()) { modoUso = "Paso Ambulancia"; }
        // normal de nuevo
        if (!botonPeatonalsolicitado && !estaEnModalidadMadrugada && !isAmbulancePassingThrough()) { modoUso = "Ciclo Normal   "; }

        lastime = millis();

        lcd.setCursor(0, 0);
        lcd.print("---- SMART CITY ----");

        lcd.setCursor(0, 1);
        lcd.print("Modo de Semaforo:");

        lcd.setCursor(0, 2);
        lcd.print(modoUso);
        
        // lcd.setCursor(5, 0); lcd.print(vLDR1);
        // lcd.setCursor(5, 1); lcd.print(vLDR2);
        // lcd.setCursor(5, 2); lcd.print(vCO2);
        // lcd.setCursor(3, 3); lcd.print(1 * vP1);
        // lcd.setCursor(8, 3); lcd.print(1 * vP2);
        // lcd.setCursor(15, 1); lcd.print(1 * vCNY1);
        // lcd.setCursor(17, 1); lcd.print(1 * vCNY2);
        // lcd.setCursor(19, 1); lcd.print(1 * vCNY3);
        // lcd.setCursor(15, 3); lcd.print(1 * vCNY4);
        // lcd.setCursor(17, 3); lcd.print(1 * vCNY5);
        // lcd.setCursor(19, 3); lcd.print(1 * vCNY6);

        // mostrarEstadoLCD();
    }

    calculateBrightnessLevel();

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
                state = !isAmbulancePassingThrough() ? VERDE_ROJO : PASO_AMBULANCIA_VIA1;
            }
            trafficLevelManager1.calculateAdditionalTime();
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case VERDE_ROJO:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)false, (int)false, (int)true);
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, (int)false, (int)false);

            sevenSegmentManager1.print(((trafficLevelManager1.getAdditionalTime() + duracionVerde + DURACION_VERDE_T + DURACION_AMARILLO + 1000) - (currentMillis - previousMillis)) / 1000);
            sevenSegmentManager2.print(-1);

            if (currentMillis - previousMillis >= trafficLevelManager1.getAdditionalTime() + duracionVerde) {
                previousMillis = currentMillis;
                state = !isAmbulancePassingThrough() ? VERDE_T_ROJO : PASO_AMBULANCIA_VIA1;
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
                state = !isAmbulancePassingThrough() ? AMARILLO_ROJO : PASO_AMBULANCIA_VIA1;

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
                state = !isAmbulancePassingThrough() ? ROJO_ROJO_V2 : PASO_AMBULANCIA_VIA1;

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
            sevenSegmentManager2.print(((trafficLevelManager2.getAdditionalTime() + duracionVerde + DURACION_VERDE_T + DURACION_AMARILLO + 1000) - (currentMillis - previousMillis)) / 1000);

            if (currentMillis - previousMillis >= trafficLevelManager2.getAdditionalTime() + duracionVerde) {
                previousMillis = currentMillis;
                state = ROJO_VERDE_T;
                trafficLevelManager2.reset();
                duracionVerde = 3000;
                botonPeatonalsolicitado = false;
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
                state = !isAmbulancePassingThrough() ? ROJO_ROJO : PASO_AMBULANCIA_VIA1;
            }
            trafficLevelManager1.calculateAdditionalTime();
            trafficLevelManager2.calculateAdditionalTime();
            break;

        case AMARILLO_T_ROJO_T:
            currentBrightness = 255;
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)false, ((currentMillis - previousMillis) / (2 * DURACION_TITILACION)) % 2, (int)false); // Luz amarilla titilando
            actualizarSemaforo(S2LR, S2LA, S2LV, ((currentMillis - previousMillis) / (2 * DURACION_TITILACION)) % 2, (int)false, (int)false); // Luz roja titilando
            state = estaEnModalidadMadrugada ? AMARILLO_T_ROJO_T : ROJO_ROJO;
            break;

        case PASO_AMBULANCIA_VIA1:
            actualizarSemaforo(S1LR, S1LA, S1LV, (int)false, ((currentMillis - previousMillis) / (2 * DURACION_TITILACION)) % 2, (int)true); // Luz amarilla titilando
            actualizarSemaforo(S2LR, S2LA, S2LV, (int)true, ((currentMillis - previousMillis) / DURACION_TITILACION) % 2, (int)false); // Luz roja titilando
            state = isAmbulancePassingThrough() ? PASO_AMBULANCIA_VIA1 : ROJO_ROJO_V2;
            break;
    }
}
