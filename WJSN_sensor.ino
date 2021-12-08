/*
 *  Nombre: WJSN_sensor.ino
 *  Autor: Pablo Linares Serrano
 *  
 *  Descripción: Implementación de un nodo sensor WJSN empleando la biblioteca WJSN.h
 */
// inclusión de bibliotecas. La biblioteca VW está incluida en WJSN.h
#include <WJSN.h>
#include <dht11.h>

// Definimos las etiquetas de los canales
#define ID_TEM 1
#define ID_HUM 2

// Definición de parámetros
#define ID 3
#define INCREMENTO 1
#define DHT11PIN 3

// Declaración de los punteros para pasar las medias al PGD
uint8_t bandera;
uint8_t tam;
uint8_t datos[4];
uint8_t estado;
// Declaración del objeto nodo WJSN
nodoSensor sensor;
// Declaración del objeto sensor de humedad/temperatura
dht11 DHT11;
void setup() {
  sensor.inicializarNS(ID, INCREMENTO, &bandera, &tam, datos);
  Serial.begin(9600);
}

void loop() {
  sensor.actualizarNS();
  if (0 == bandera){
    //medimos
    if(DHT11.read(DHT11PIN)){
      datos[0] = ID_TEM;
      datos[1] = (uint8_t) DHT11.temperature;
      datos[2] = ID_HUM;
      datos[3] = (uint8_t) DHT11.humidity;
      tam = 2;
      bandera = 1;
      Serial.print("temperatura: ");
      Serial.println(String(DHT11.temperature));
      Serial.print("humedad: ");
      Serial.println(String(DHT11.humidity));
    }
  }
}
