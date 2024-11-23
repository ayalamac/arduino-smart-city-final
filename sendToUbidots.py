import serial
import requests
import time

# Configuración del puerto serial
API_URL = "https://industrial.api.ubidots.com/api/v1.6/variables/67416a709b90921f67fb9529/values"  # Cambia por tu API REST

# Crear conexión al puerto serial
try:
    ser = serial.serial_for_url('rfc2217://localhost:4000', baudrate=9600)
    print(f"Conectado al puerto serial")
except Exception as e:
    print(f"Error al conectar al puerto serial: {e}")
    exit()

# Función para enviar datos a la API REST
def enviar_a_api(data):
    try:
        payload = {"value": data}  # Estructura del JSON
        headers = {"Content-Type": "application/json", "X-Auth-Token": "BBUS-B1oCLGfzDsvpgojjIBslcMk4OP5ydO"}
        response = requests.post(API_URL, json=payload, headers=headers)

        if response.status_code == 200:
            print(f"Datos enviados exitosamente: {data}")
        else:
            print(f"Error al enviar datos: {response.status_code}, {response.text}")
    except Exception as e:
        print(f"Error en la conexión con la API: {e}")

# Bucle para leer el puerto serial y enviar datos
try:
    while True:
        if ser.in_waiting > 0:  # Verifica si hay datos disponibles en el puerto serial
            line = ser.readline().decode('utf-8').strip()  # Leer línea del puerto serial
            if line:  # Si la línea no está vacía
                print(f"Datos recibidos: {line}")
                # enviar_a_api(line)  # Enviar datos a la API
        time.sleep(0.1)  # Evitar alta carga de CPU
except KeyboardInterrupt:
    print("\nInterrupción por teclado. Cerrando programa...")
finally:
    ser.close()  # Cerrar conexión serial