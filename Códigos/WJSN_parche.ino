/*
 * Nombre: WJSN_parche.ino
 * 
 * Autor: Pablo Linares Serrano
 * 
 * Descripción: Código para solucionar la incopatibilidad de la bibliotecha Ethernet y VirtualWire
 * La idea es que la pasarela envía a través del puerto serie lo que quiera transmitir por radio
 * y otra placa diferente lo hará. El puente es a nivel físico prácticamente y no influye en la conformación
 * de los paquetes.
 */
 
#include <VirtualWire.h>

#define TASA 2000
#define TX_PIN 4

byte buf[VW_MAX_MESSAGE_LEN];
byte buflen = VW_MAX_MESSAGE_LEN;
byte buf2[VW_MAX_MESSAGE_LEN];
byte buflen2 = VW_MAX_MESSAGE_LEN;
int tam;
    
void setup() {
   Serial.begin(9600);
   // configuramos VW
   vw_setup(TASA);  // Bits per sec
   vw_set_ptt_inverted(true); // Required for DR3100

   // inicializamos el receptor
   vw_set_rx_pin (5);
   vw_rx_start();       // Start the receiver PLL running

   // inicializamos el transmisor
   vw_set_tx_pin(4);
}

void loop() {
  buflen = VW_MAX_MESSAGE_LEN;
  buflen2 = VW_MAX_MESSAGE_LEN;
  if(Serial.available()){
    digitalWrite(13, true); // Flash a light to show transmitting
    tam = Serial.readBytes(buf, buflen);
    vw_send((uint8_t *)buf, tam);
    vw_wait_tx(); // Wait until the whole message is gone
    digitalWrite(13, false);
  }
  if (vw_get_message(buf2, &buflen2)) // Non-blocking
  {
      Serial.write(buf2, buflen2);
      delay(1500);
  }
  delay(100);
}
