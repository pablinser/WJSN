/*
	Autor: Pablo Linares Serrano 
	Nombre: Proyecto WJSN
	Descripcion: implementacion de las clases necesarias para el funcionamiento de los protocolos PER, CEPER y PGD diseñados
	por el mismo autor para el proyecto WJSN.

	WJSN by Pablo Linares Serrano is licensed under Attribution-NonCommercial-ShareAlike 4.0 International
*/

#include "WJSN.h"

// Implementación de los métodos de la clase nodoPasarela

nodoPasarela::nodoPasarela(){
	// Constructor del nodo pasarela
	_puertoAsociado = 0;
	_puerto = 0;
	_estado = ESPERA;
	_tamTablaRD = 0;
	_id = 0;
	_incrementoTemporal = 1;
	_contSegundos = 0;
	_contAnuncio = T_ANUNCIO;
	_UDP;

}

void nodoPasarela::inicializarNP(uint8_t id, IPAddress ip, byte* mac, unsigned int puerto, uint8_t incrementoTemporal){
	// Inicializa un nodo pasarela, estableciendo los valores apropiados en sus campos en tiempo de ejecución.
	Serial.begin(9600);

    // inicializacion de los pines de los led
	pinMode(LED_RX_PIN, OUTPUT);
	pinMode(LED_TX_PIN, OUTPUT);
	pinMode(LED_ASOCIADO_PIN, OUTPUT);
	pinMode(LED_ESPERA_PIN, OUTPUT);

	// asignación de valores a variables
	_incrementoTemporal = incrementoTemporal;
	_id = id;
	_estado = ESPERA;
	_tamTablaRD = 0;
	_contSegundos = 0;
	_contAnuncio = T_ANUNCIO;

	// ponemos a cero las variables propias de los nodos tipo sensor:
	_ip = ip;
	_puerto = puerto;

    // inicializamos el puerto ethernet
	Ethernet.begin(mac, _ip);

	//inicializamos el puerto UDP
	_UDP.begin(puerto);
}

void nodoPasarela::actualizarNP(){
	// Actualiza todos los contadores necesarios,
	// Comprueba si se han recibido mensajes y responde de ser necesario
	// Comprueba si es necesario enviar nuevos mensajes
	uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen;
    uint8_t buf2[VW_MAX_MESSAGE_LEN];
    uint8_t buflen2;
    char ReplyBuffer[] = "ACK"; 
    int packetSize;
    mensaje Msg;
    buflen = VW_MAX_MESSAGE_LEN;
    buflen2 = VW_MAX_MESSAGE_LEN;
    // Leemos los paquetes que se hayan recibido por ethernet
	packetSize = _UDP.parsePacket();
  	if (packetSize) {
  		
  		// Realizamos la asociación
  		_UDP.read(buf,buflen);
  		_ipAsociado = _UDP.remoteIP();
  		_puertoAsociado = _UDP.remotePort();
  		_estado = ASOCIADO;

  		//contestamos a la asociación con ACK
  		_UDP.beginPacket(_UDP.remoteIP(), _UDP.remotePort());
  		_UDP.write(ReplyBuffer);
  		_UDP.endPacket();
  		delay(50);
  	}

    if (Serial.available()) // Non-blocking
    {
    	// Encendemos el led de mensaje recibido mientras se procesa
    	digitalWrite(LED_RX_PIN, HIGH);

    	// Leemos el mensaje
    	buflen2 = Serial.readBytes(buf2, buflen2);
    	Msg.setMsg(buf2, buflen2);

    	// Procesamos el mensaje
    	procesarPER(Msg);

    	// Apagamos el led
    	digitalWrite(LED_RX_PIN, LOW);
    }
    // Reflejamos el estado en los leds
    switch (_estado) {
    	case ESPERA:
    		// Encendemos el led del estado ESPERA y apgamos ASOCIADO
	    	digitalWrite(LED_ASOCIADO_PIN, LOW);
	    	digitalWrite(LED_ESPERA_PIN, HIGH);
	    break;
	    case ASOCIADO:
	    	// Encendemos el led de estado ASOCIADO y apagamos ESPERA
			digitalWrite(LED_ASOCIADO_PIN, HIGH);
	    	digitalWrite(LED_ESPERA_PIN, LOW);
	    break;
	}
    // Actualizamos el contador de segundos
	_contSegundos = _contSegundos + _incrementoTemporal;
	if(_contSegundos > 59){
		_contSegundos = _contSegundos - 60;
		if (ASOCIADO == _estado) {
			// En caso de estar asociado, enviamos ANUNCIAR cuando toque
			_contAnuncio--;
			if(0 == _contAnuncio){
				_contAnuncio = T_ANUNCIO;
				anunciar();
			}
		}
	}
	delay(1000*_incrementoTemporal);
}

int nodoPasarela::getEstado(){
	return _estado;
}

// Métodos relacionados con el protocolo CEPER

void nodoPasarela::procesarCEPER(mensaje Msg){
	// Aplica los cambios necesarios en el nodo según el protocolo CEPER y responde según el protocol
	switch (Msg.getTipo()) {
		case DESCUBRIR:
		/*
			Operaciones que se realizarán si se recibe un mensaje DESCUBRIR:
			- Enviar inmediatamente un mensaje de tipo ANUNCIAR
		*/
			if(_estado == ASOCIADO){
				anunciar();
			}
			break;
			
	    case ANUNCIAR:
	    /*
			Operaciones que se realizaran si se recibe un mensaje ANUNCIAR:
			- Comprobar si existe la pasarela a la que hace referencia en la tabla de reenvío
			- Es caso de que no exista, se responde con un mensaje tipo ASOCIAR.
		*/
			break;

	    case ASOCIAR:
	    /*
			Operaciones que se realizaran si se recibe un mensaje ASOCIAR:
			- Añadir en la tabla de reenvio descendente la entrada correspondiente al emisor del mensaje
			- Responder con un ACK con el mismo id para confirmar la operacion.
		*/
			if (_estado = ASOCIADO){
				// Añadimos la dirección de origen a la tabla de reenvío descendente:
				// Hay que comprobar que la entrada no esté ya registrada y que no se exceda el tamaño reservado.
				if(comprobarRD(Msg.getOrig(), Msg.getPasarela()) && TAM_TABLA_RD > _tamTablaRD){
					_tablaRD[_tamTablaRD][0] = Msg.getOrig();
					_tablaRD[_tamTablaRD][1] = Msg.getPasarela();
					_tamTablaRD++;
					// Se responde con un mensaje ACK:
					ack(Msg);

				}else if(!comprobarRD(Msg.getOrig(), Msg.getPasarela())){
				    ack(Msg);
				}
			}
			break;

	    case ACK:
			// No hacemos nada, los nodos pasarela no se asocian
			break;

		case CONFIGURAR:
			// Aquí se actualizaría la configuración del nodo.
			// No se va a implementar esta funcionalidad: descartamos el mensaje.
			break;

	    default:
	    	// Si llega cualquier mensaje que no esté definido se descarta.
	    	break;
	}
}

void nodoPasarela::anunciar(){
	// Envía un mensaje de tipo ANUNCIAR
	mensaje Msg;
	uint8_t* tabla;

	tabla= (uint8_t*) malloc(9);
	Msg = mensaje(tabla, 9);
	Msg.setSigs(DIF);
	Msg.setDest(DD);
	Msg.setOrig(_id);
	Msg.setProt(1);
	Msg.setTipo(ANUNCIAR);
	Msg.setNsalto(0);
	Msg.setPasarela(_id);
	Msg.setId(random(1, 255));
	Msg.setLong();
	Msg.enviarS();
	free(tabla);
}

void nodoPasarela::ack(mensaje Msg){
	// Contesta a un mensaje de tipo ASOCIAR con ACK
	Msg.setSigs(Msg.getOrig());
	Msg.setDest(Msg.getOrig());
	Msg.setOrig(_id);
	Msg.setTipo(ACK);
	Msg.enviarS();
}

int nodoPasarela::comprobarRD(uint8_t ID, uint8_t pasarela){
	// devuelve 0 si el ID está en la tablaRD
	int result;
	result = 1;
	for(int i=0; i<_tamTablaRD; i++){
	    if(ID == _tablaRD[i][0] && _tablaRD[i][1] == pasarela){
	        result = 0;
	    }
	}
	return result;
}

// Métodos relacionados con el protocolo PER

void nodoPasarela::procesarPER(mensaje Msg){
	// Decide si el mensaje recibido se acepta o se elimina.
	// Al ser una pasarela, no reenviará mensajes, solo los emite
	// o los recibe si es el destino.
	if(Msg.getSigs() == _id || Msg.getSigs() == DIF){
		if(Msg.getDest() == _id || Msg.getDest() == DD){
			aceptarPER(Msg);
		}
	}
}

void nodoPasarela::aceptarPER(mensaje Msg){
	// Da por aceptado un mensaje de tipo per y llama a los protocolos
	// de capas superiores para que procesen el mensaje.
	if (Msg.getProt() == 1){
		procesarCEPER(Msg);
	}
	if (Msg.getProt() == 2){
		procesarPGD(Msg);
	}
}


void nodoPasarela::reenvioD(mensaje Msg){
	// Hace el reenvío descendente de un mensaje
	// En este caso no se comprueba la pasarela origen porque
	// es el propio nodo
	for(int i=0; i<_tamTablaRD; i++){
		if(_tablaRD[i][1] == Msg.getOrig()){
	    	Msg.setSigs(_tablaRD[i][0]);
	    	Msg.enviarS();
		}
	}
}


// Métodos relacionados con el protocolo PGD

void nodoPasarela::procesarPGD(mensaje Msg){
	// Carga un mensaje PGD (plano de datos) sobre UDP para
	// enviarlo al servidor(es) asociado(s). (En nuestro caso siempre será un único servidor)
	uint8_t* puntero;
	uint8_t tam;
	puntero = Msg.imprimirCPG(&tam);
	_UDP.begin(_puerto);
	_UDP.beginPacket(_ipAsociado, _puertoAsociado);
	_UDP.write(puntero, tam);
	_UDP.endPacket();
	delay(50);
	free(puntero);
}

// Implementación de los métodos de la clase nodoSensor

nodoSensor::nodoSensor(){
	// Constructor del nodo sensor.
	_estado = ESPERA;
	_tamTablaRD = 0;
	_tamTablaRA = 0;
	_id = 0;
	_incrementoTemporal = 1;
	_contSegundos = 0;
	_contAnuncio = T_ANUNCIO;
	_contDescubrir = T_DESCUBRIR;
	_contMedicion = T_MEDIDA;
	_bandera = NULL;
	_tam = NULL;
	_datos = NULL;
}

void nodoSensor::inicializarNS(uint8_t id, uint8_t incrementoTemporal, uint8_t* bandera, uint8_t* tam, uint8_t* datos){
	// Inicializa un nodo sensor otorgando los parámetros necesarios a la instancia en tiempo de ejecución.
	// inicializacion de los pines de los led
	pinMode(LED_RX_PIN, OUTPUT);
	pinMode(LED_TX_PIN, OUTPUT);
	pinMode(LED_ASOCIADO_PIN, OUTPUT);
	pinMode(LED_ESPERA_PIN, OUTPUT);

	// asignación de valores a variables
	_incrementoTemporal = incrementoTemporal;
	_id = id;
	_estado = ESPERA;
	_tamTablaRD = 0;
	_tamTablaRA = 0;
	_contSegundos = 0;
	_contDescubrir = T_DESCUBRIR;
	_contAnuncio = T_ANUNCIO;
	_contMedicion = T_MEDIDA;

    // Interfaz con el programa que toma medidas
    _bandera = bandera;
    _tam = tam;
    _datos = datos;

    // Parametros y funciones de VirtualWire
    vw_setup(2000);  // Bits per sec
 	vw_set_ptt_inverted(true); // Required for DR3100
 	
 	// inicializamos el receptor
 	vw_set_rx_pin (RX_PIN);
 	vw_rx_start();       // Start the receiver PLL running
 	
 	// inicializamos el transmisor
 	vw_set_tx_pin(TX_PIN);
}

void nodoSensor::actualizarNS(){
	// Actualiza todos los contadores necesarios,
	// Comprueba si se han recibido mensajes y responde de ser necesario
	// Comprueba si es necesario enviar nuevos mensajes

	// Declaramos el buffer de lectura de mensajes
	uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    mensaje Msg;
    //uint8_t tam;
    uint8_t* medida;
    uint8_t tamMedida;

    // Leemos los mensajes que se hayan recibido
    if (vw_get_message(buf, &buflen)) // Non-blocking
    {	
    	// Encendemos el led de mensaje recibido mientras se procesa
    	digitalWrite(LED_RX_PIN, HIGH);
    	Msg.setMsg(buf, buflen);
    	procesarPER(Msg);

    	// Apagamos el led
    	digitalWrite(LED_RX_PIN, LOW);
    }
    switch (_estado) {
    	case ESPERA:
    		// Encendemos el led del estado ESPERA y apgamos ASOCIADO
	    	digitalWrite(LED_ASOCIADO_PIN, LOW);
	    	digitalWrite(LED_ESPERA_PIN, HIGH);
	    break;
	    case ASOCIADO:
	    	// Encendemos el led de estado ASOCIADO y apagamos ESPERA
			digitalWrite(LED_ASOCIADO_PIN, HIGH);
	    	digitalWrite(LED_ESPERA_PIN, LOW);
	    break;
	}
    // Actualizamos el contador de segundos
	_contSegundos = _contSegundos + _incrementoTemporal;
	if(_contSegundos > 59){
		// Si se cumple un minuto, ajustamos el valor del contador y 
		// disminuimos los demas contadores (que cuentan minutos)
		_contSegundos = _contSegundos - 60;
		switch (_estado){
	    	case ESPERA:
				_contDescubrir--;
				
				if(0 == _contDescubrir){
					// Si el contador llega a cero lo reseteamos
					// y enviamos un mensaje descubrir
					_contDescubrir = T_DESCUBRIR;
					descubrir();
				}
			break;

			case ASOCIADO:
			    _contAnuncio--;
				_contMedicion--;
				
			    if(0 ==_contMedicion){
			    	// Comprueba que el usuario
			    	// haya indicado que hay una 
			    	// nueva medida disponible y la 
			    	// envía en caso afirmativo.
			        _contMedicion = T_MEDIDA;
			        medida = medir(&tamMedida);
			        Msg.setMsg(medida, tamMedida);
			        envioA(Msg);
			        free(medida);
			    }   
			    if(0 == _contAnuncio){
			    	//reseteamos el contador y 
			    	// enviamos un anuncio
			    	_contAnuncio = T_ANUNCIO;
			    	anunciar();
			    }
			break;

	    	default:
	      	break;
		}
	}
	delay(1000*_incrementoTemporal);
}

int nodoSensor::getEstado(){
	return _estado;
}

void nodoSensor::procesarCEPER(mensaje Msg){
	switch (Msg.getTipo()) {
		case DESCUBRIR:
		/*
			Operaciones que se realizarán si se recibe un mensaje DESCUBRIR:
			- Enviar inmediatamente un mensaje de tipo ANUNCIAR
		*/
			if (_estado == ASOCIADO){
				anunciar();
			}
			break;
			
	    case ANUNCIAR:
	    /*
			Operaciones que se realizaran si se recibe un mensaje ANUNCIAR:
			- Comprobar si existe la pasarela a la que hace referencia en la tabla de reenvío
			- Es caso de que no exista, se responde con un mensaje tipo ASOCIAR.
		*/
			if(comprobarRA(Msg.getOrig()) && _tamTablaRA < TAM_TABLA_RA){
				// Enviamos un mensaje asociar
				asociar(Msg);
			}
			break;

	    case ASOCIAR:
	    /*
			Operaciones que se realizaran si se recibe un mensaje ASOCIAR:
			- Añadir en la tabla de reenvio descendente la entrada correspondiente al emisor del mensaje
			- Responder con un ACK con el mismo id para confirmar la operacion.
		*/
			// Añadimos la dirección de origen a la tabla de reenvío descendente:
			// Hay que comprobar que la entrada no esté ya registrada y que no se exceda el tamaño reservado.

			if(comprobarRD(Msg.getOrig(), Msg.getPasarela()) && TAM_TABLA_RD > _tamTablaRD){
				// añaidmos al remitente a la tablaRD
				_tablaRD[_tamTablaRD][0] = Msg.getOrig();
				_tablaRD[_tamTablaRD][1] = Msg.getPasarela();
				_tamTablaRD++;
				// Se responde con un mensaje ACK:
				ack(Msg);
			}else if(!comprobarRD(Msg.getOrig(), Msg.getPasarela())){
				// Significará que no llegó el anterior ACK, pero ya está en la tabla
			    ack(Msg);
			}
			break;
	    case ACK:
	    	// si se fuese a desbordar la tabla, la reescribimos
	    	if(_tamTablaRA >= TAM_TABLA_RA){
	    		_tamTablaRA = 0;
	    	}
	    	// Comprobamos que haya espacio en la tabla y que la pasarela en cuestión no esté ya almacenada
	    	if(_tamTablaRA < TAM_TABLA_RA && comprobarRA(Msg.getPasarela())){

	    		// Añadimos la pasarela confirmada a la tablaRA
	    		_tablaRA[_tamTablaRA] = Msg.getPasarela();
	    		_tablaRA[_tamTablaRA+1] = Msg.getOrig();
	    		_tablaRA[_tamTablaRA+2] = Msg.getNsalto()+1;
				_tamTablaRA++;
				// Pasamos a estado ASOCIADO
				_estado = ASOCIADO;
			}
			break;
		case CONFIGURAR:
			// Aquí se actualizaría la configuración del nodo.
			// No se va a implementar esta funcionalidad: descartamos el mensaje.
			break;

	    default:
	    	// Si llega cualquier mensaje que no esté definido se descarta (no se hace nada).
	    	break;
	}
}

void nodoSensor::anunciar(){
	// Envía un mensaje CEPER de tipo ANUNCIAR
	mensaje Msg;
	uint8_t* tabla;

	tabla= (uint8_t*) malloc(9);
	Msg = mensaje(tabla, 9);
	Msg.setSigs(DIF);
	Msg.setDest(DD);
	Msg.setOrig(_id);
	Msg.setProt(1);
	Msg.setTipo(ANUNCIAR);
	Msg.setId(random(1, 255));
	Msg.setLong();

	for(int i=0; i<_tamTablaRA; i++){
	    Msg.setPasarela(_tablaRA[i*3]);
	    Msg.setNsalto(_tablaRA[i*3+2]);
		Msg.enviar();
	}
	free(tabla);
}

void nodoSensor::descubrir(){
	// Envía un mensaje CEPER de tipo DESCUBRIR
	mensaje Msg;
	uint8_t* tabla;

	tabla= (uint8_t*) malloc(9);
	Msg = mensaje(tabla, 9);
	Msg.setSigs(DIF);
	Msg.setDest(DD);
	Msg.setOrig(_id);
	Msg.setProt(1);
	Msg.setTipo(DESCUBRIR);
	Msg.setPasarela(0);
	Msg.setNsalto(0);
	Msg.setId(random(1, 255));
	Msg.setLong();
	Msg.enviar();
	free(tabla);
}

int nodoSensor::comprobarRA(uint8_t ID){
	// devuelve 0 si el ID está en la tablaRA
	// devuelve 1 si el ID NO está en la tablaRA
	int result;
	result = 1;
	for(int i=0; i<_tamTablaRA; i++){
	    if(ID == _tablaRA[i]){
	        result = 0;
	    }
	}
	return result;
}

int nodoSensor::comprobarRD(uint8_t ID, uint8_t pasarela){
	// devuelve 0 si el ID está en la tablaRD
	// devuelve 1 si el ID NO está en la tablaRD
	int result;
	result = 1;
	for(int i=0; i<_tamTablaRD; i++){
	    if(ID == _tablaRD[i][0] && pasarela == _tablaRD[i][1]){
	        result = 0;
	    }
	}
	return result;
}

void nodoSensor::ack(mensaje Msg){
	// Responde a un mensaje de tipo ASOCIAR con un mensaje de tipo ACK
	Msg.setSigs(Msg.getOrig());
	Msg.setDest(Msg.getOrig());
	Msg.setOrig(_id);
	Msg.setTipo(ACK);
	Msg.enviar();
}

void nodoSensor::asociar(mensaje Msg){
	//Responde a un mensaje de tipo anunciar con un mensaje de tipo asociar.
	Msg.setSigs(Msg.getOrig());
	Msg.setDest(Msg.getOrig());
	Msg.setOrig(_id);
	Msg.setTipo(ASOCIAR);
	Msg.enviar();
}

// Métodos relacionados con el protocolo PER

void nodoSensor::procesarPER(mensaje Msg){
	// Ejecuta el protocolo de reenvío PER, decidiendo si se reenvía
	if(Msg.getSigs() == _id || Msg.getSigs() == DIF){
		if(Msg.getDest() == _id || Msg.getDest() == DD){
			aceptarPER(Msg);
		}else{
			if(Msg.getDest() == DIF){
				reenvioD(Msg);
				aceptarPER(Msg);
			}else{
				
				reenvioA(Msg);
			}
		}
	}
}

void nodoSensor::aceptarPER(mensaje Msg){
	if(Msg.getProt() == 1){
	    procesarCEPER(Msg);
	}
	// Solo se deberían recibir mensajes CEPER. 
	// en cualquier otro caso se descarta el mensaje
	// Es decir, no hacemos nada.
}

void nodoSensor::envioA(mensaje Msg){
	// Envía un mensaje hacia todas las pasarelas descubiertas
	for(int i=0; i<_tamTablaRA; i++){
	    Msg.setSigs(_tablaRA[i*3+1]);
	    Msg.setDest(_tablaRA[i*3]);
	    Msg.enviar();
	}
}

void nodoSensor::reenvioA(mensaje Msg){
	// Reenvía un mensaje hacia la pasarela en cuestión
	for(int i=0; i<_tamTablaRA; i++){
		if(Msg.getDest() == _tablaRA[i*3]){
	    	Msg.setSigs(_tablaRA[i*3+1]);
	    	Msg.enviar();
		}
	}
}

void nodoSensor::reenvioD(mensaje Msg){
	// Reenvía un mensaje hacia todos los nodos asociados 
	// relativos a la pasarela que es origen del mensaje
	for(int i=0; i<_tamTablaRD; i++){
		if(_tablaRD[i][1] == Msg.getOrig()){ 
	    	Msg.setSigs(_tablaRD[i][0]);
	    	Msg.enviar();
		}
	}
}

uint8_t* nodoSensor::medir(uint8_t* tam){
	// Toma las medidas almacenadas en la dirección de memoria apropiada y las
	// carga en un mensaje PGD encapsulado en un mensaje PER, reservando para ello memoria dinámica
	uint8_t* result;
	mensaje Msg;
	if(*(_bandera) == 1){
		*(tam) = 5+2*(*(_tam));
	    result = (uint8_t*) malloc(*(tam));
	    Msg = mensaje(result, *(tam));
	    Msg.setSigs(0);
	    Msg.setDest(0);
	    Msg.setOrig(_id);
	    Msg.setProt(2);
	    result[4] = *(_tam);
	    for(int i=0; i<*(_tam); i++){
	        result[5+i*2] = _datos[i*2];
	        result[5+i*2+1] = _datos[i*2+1];
	    }
	}else{
		result = NULL;
		*(tam) = 0;
	}
	*(_bandera) = 0;
	return result;
}

// Implementación de los métodos de la clase mensaje

mensaje::mensaje(){
	// COnstructor de la clase mensaje
	_mensaje = NULL;
	_tam = 0;
}
mensaje::mensaje(uint8_t* buff, uint8_t tam){
	// Constructor de la clase mensjae pensado para tiempo de ejecución.
	_mensaje = buff;
	_tam = tam;
}
void mensaje::enviar(){
	// Envía un mensaje por radio
	digitalWrite(LED_TX_PIN, HIGH);
	// Para reducir colisiones, esperamos un tiempo aleatorio.
	delay(random(0,5000));
	vw_rx_stop();
	vw_send(_mensaje, _tam);
    vw_wait_tx();
    digitalWrite(LED_TX_PIN, LOW);
    // Reactivamos el rx
    vw_rx_start();
    // Aseguramos 5 segundos de silencio
    delay(5000);
}

void mensaje::enviarS(){
	// Envía un mensaje por el puerto serie
	digitalWrite(LED_TX_PIN, HIGH);
    // para evitar colisiones, esperamos un tiempo aleatorio.
    delay(random(0,5000));
    Serial.write(_mensaje, _tam);
    // Damos tiempo para que el "parche" reciba el mensaje y lo reenvíe.
    delay(500);
    digitalWrite(LED_TX_PIN, LOW);
}

void mensaje::setSigs(uint8_t sigs){
	if(0<_tam){
	    _mensaje[0] = sigs;
	}
}

void mensaje::setDest(uint8_t dest){
	if(1<_tam){
	    _mensaje[1] = dest;
	}
}

void mensaje::setOrig(uint8_t orig){
	if(2<_tam){
	    _mensaje[2] = orig;
	}
}

void mensaje::setProt(uint8_t prot){
	if(3<_tam){
	    _mensaje[3] = prot;
	}
}
void mensaje::setMsg(uint8_t* buff, uint8_t tam){
	_mensaje = buff;
	_tam = tam;
}
uint8_t mensaje::getSigs(){
	return _mensaje[0];
}
uint8_t mensaje::getDest(){
	return _mensaje[1];
}
uint8_t mensaje::getOrig(){
	return _mensaje[2];
}
uint8_t mensaje::getProt(){
	return _mensaje[3];
}
uint8_t mensaje::getTam(){
	return _tam;
}

// Métodos CEPER
void mensaje::setTipo(uint8_t tipo){
	_mensaje[4] = tipo;
}
void mensaje::setNsalto(uint8_t nsalto){
	_mensaje[5] = nsalto;
}
void mensaje::setPasarela(uint8_t pasarela){
	_mensaje[6] = pasarela;
}
void mensaje::setId(uint8_t id){
	_mensaje[7] = id;
}
void mensaje::setLong(){
	_mensaje[8] = 0;
}
uint8_t mensaje::getTipo(){
	return _mensaje[4];
}
uint8_t mensaje::getNsalto(){
	return _mensaje[5];
}
uint8_t mensaje::getPasarela(){
	return _mensaje[6];
}
uint8_t mensaje::getId(){
	return _mensaje[7];
}
uint8_t* mensaje::imprimirCPG(uint8_t* tam){
	// devuelve una tabla dinámica con el mensaje PGD precedido del origen, listo para montarlo sobre UDP.
	uint8_t* result;
	uint8_t N = 0;
	if (2 == _mensaje[3]){
		N = _mensaje[4];
		*(tam) = N*2+2;
		result = (uint8_t*) malloc(N*2+2);
		result[0] = _mensaje[2];
		result[1] = N;

		for(int i=0; i<N; i++){
		    result[i*2+2] = _mensaje[i*2+5];
		    result[i*2+3] = _mensaje[i*2+6];
		}
	}else{
		*(tam) = 0;
		result = NULL;
	}

	return result;
}