/*
 *  Nombre: WJSN_sensor_debug.ino
 *  Autor: Pablo Linares Serrano
 *  
 *  Descripción: Código para programar un nodo sensor como debugger de otro nodo sensor.
 *  Permite enviarle mensajes y leer los que envíe.
 */

// inclusión de bibliotecas. La biblioteca VW está incluida en WJSN.h
#include <WJSN.h>
#include <VirtualWire.h>
#define TX_PIN 4
#define RX_PIN 5
#define LED1 13
#define LED2 8
#define ID_LOC 1
#define ID_REM 2
#define DIF 255
#define DD 254

// Definimos los tipos de mensajes CEPER
#define ANUNCIAR 1
#define CONFIGURAR 2
#define DESCUBRIR 3
#define ASOCIAR 4
#define ACK 5
#define RECHAZAR 6

char rx; 
uint8_t tam;
uint8_t buf[200];
uint8_t buflen = 200;
mensaje Msg(buf, buflen);

void setup() {
  Serial.begin(9600);
  Serial.println("Setup");
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT); 
  rx = '3';
  tam = 0;
  vw_setup(2000);  // Bits per sec
  vw_set_ptt_inverted(true); // Required for DR3100

  // inicializamos el receptor
  vw_set_rx_pin (RX_PIN);
  vw_rx_start();       // Start the receiver PLL running
  // inicializamos el transmisor
  vw_set_tx_pin(TX_PIN);
}

void loop() {
  buflen = 200;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    int i;
    digitalWrite(LED1, HIGH);
    Serial.println("Got message: ");
    for (i = 0; i < buflen; i++)
    {
      Serial.print(String(buf[i]));
      Serial.print(" ");
    }
    Serial.println("");
    Serial.println("Got 2: ");
    // Message with a good checksum received, dump it.
    //Serial.println("Got: ");
    Serial.write(buf, buflen);
    Serial.println("");
    Msg.setMsg(buf, buflen);
    Serial.println("SIGS: " + String(Msg.getSigs()));
    Serial.println("DEST: " + String(Msg.getDest()));
    Serial.println("ORIG: " + String(Msg.getOrig()));
    if (Msg.getProt()== 1){
      Serial.println("Protocolo encapsulado: CEPER");
      switch (Msg.getTipo()){
        case ANUNCIAR:
          Serial.println("Tipo: ANUNCIAR");
          break;
        case CONFIGURAR:
          Serial.println("Tipo: CONFIGURAR");
          break;
        case DESCUBRIR:
          Serial.println("Tipo: DESCUBRIR");
          break;
        case ASOCIAR:
          Serial.println("Tipo: ASOCIAR");
          break;
        case ACK:
          Serial.println("Tipo: ACK");
          break;
        case RECHAZAR:
          Serial.println("Tipo: RECHAZAR");
          break;
      }
    }
    Serial.println("____________________________________________");
    digitalWrite(LED1, LOW);
  }
  delay(50);
}
