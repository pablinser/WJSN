/*
 *  Nombre: WJSN_sensor.ino
 *  Autor: Pablo Linares Serrano
 *  
 *  Descripción: Implementación de un nodo sensor WJSN empleando la biblioteca WJSN.h
 */
// inclusión de bibliotecas. La biblioteca VW está incluida en WJSN.h
#include <WJSN.h>

// Definición de parámetros
#define ID 1
#define INCREMENTO 1


// Declaración del objeto nodo WJSN
nodoPasarela pasarela;
// Declaración de la mac e ip
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(169, 254, 1, 177);
unsigned int puertoloc = 5027;

void setup() {
  pasarela.inicializarNP(ID, ip, mac, puertoloc, INCREMENTO);
}

void loop() {
  pasarela.actualizarNP();
}
