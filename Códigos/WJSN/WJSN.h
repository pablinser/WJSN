#include "Arduino.h"
#include <VirtualWire.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// Definimos las etiquetas especiales:
#define DIF 255
#define DD 254

// Definimos los tipos de mensajes CEPER
#define ANUNCIAR 1
#define CONFIGURAR 2
#define DESCUBRIR 3
#define ASOCIAR 4
#define ACK 5
#define RECHAZAR 6

// Definimos los tipod e nodos
#define TIPO_S 0
#define TIPO_P 1

// Definimos los tamaños de la memoria reservada para las tablas de reenvío.
// en las implementación no superaremos los siguientes valores:
#define TAM_TABLA_RA 1
#define TAM_TABLA_RD 2

// Definimos los posibles estados del nodo
#define ESPERA 1
#define ASOCIADO 2

// Definimos los tiempos predeterminados (en minutos):
#define T_ANUNCIO 5
#define T_DESCUBRIR 1
#define T_MEDIDA 1
// No se va a implementar el olvido de asociaciones
//#define T_ASOCIACION 10

// Definimos los pines rx y tx
#define TX_PIN 4
#define RX_PIN 5

// Definimos los pines que se iluminan para cada estado y para rx o tx
#define LED_TX_PIN 6
#define LED_RX_PIN 7
#define LED_ESPERA_PIN 9
#define LED_ASOCIADO_PIN 8

// Tasa de transmisión bps
#define TASA 2000

class mensaje{
	// Clase que interpreta una cadena como un mensaje
	public:
		mensaje();
		mensaje(uint8_t* buff, uint8_t tam); // al inicializar se le otorga un espacio sobre el que operar
		// Para darle valores al mensaje hay que emplear los demás métodos
		//mensaje(uint8_t sigs, uint8_t dest, uint8_t orig, uint8_t prot, uint8_t* datos, uint8_t tam);
		// Métodos PER:
		void enviar();
		void enviarS();
		void setSigs(uint8_t sigs);
		void setDest(uint8_t dest);
		void setOrig(uint8_t orig);
		void setProt(uint8_t prot);
		void setMsg(uint8_t* datos, uint8_t tam);
		uint8_t getSigs();
		uint8_t getDest();
		uint8_t getOrig();
		uint8_t getProt();
		uint8_t getTam();
		// Métodos CEPER
		void setTipo(uint8_t tipo);
		void setNsalto(uint8_t nsalto);
		void setPasarela(uint8_t pasarela);
		void setId(uint8_t id);
		void setLong(); // pone la longitud a 0 (nunca usaremos configuración)
		uint8_t getTipo();
		uint8_t getNsalto();
		uint8_t getPasarela();
		uint8_t getId();
		// Métodos CPG
		// Función que reserva memoria dinámicamente y 
		// Devuelve el contenido CPG del mensaje junto 
		// con el origen
		uint8_t* imprimirCPG(uint8_t* tam);

	private:
		// Puntero a la cadena que contiene el mensaje
		uint8_t* _mensaje;
		// Tamaño del contenido útil de la cadena
		uint8_t _tam;
};

class nodoPasarela{
public:
	// El constructor no tiene argumentos y prácticamente es inutil. Se deben usar las funciones de inicalización
	nodoPasarela();
	
	void inicializarNP(uint8_t id, IPAddress ip, byte* mac, unsigned int puerto, uint8_t incrementoTemporal);
	void actualizarNP();
	int getEstado();

	// métodos CEPER
	void procesarCEPER(mensaje Msg);
	void anunciar();
	//void descubrir(); // Las pasarelas no necesitan descubrir
	void ack(mensaje Msg);
	//void asociar(mensaje Msg); // Las pasarelas no solicitan asociaciones
	int comprobarRD(uint8_t ID, uint8_t pasarela);

	//métodos PER
	void procesarPER(mensaje Msg);
	void aceptarPER(mensaje Msg);
	//void descartarPER(mensaje Msg); // Tiene sentido?? -> creo que no
	//void reenvioA(mensaje Msg); // Las pasarelas no reenvian ascendentemente -> lo envian sobre udp a través de procesarPGD()
	void reenvioD(mensaje Msg); // Al no haber configuración este método tiene poco sentido

	//métodos PGD
	void procesarPGD(mensaje Msg);

private:
// pareja ip/puerto del nodo remoto:
	IPAddress _ipAsociado;
	unsigned int _puertoAsociado;

// Pareja ip/puerto propia:
	IPAddress _ip;
	unsigned int _puerto;

// puntero a variable de estado:	
	uint8_t _estado;

// tabalas de reenvío y variables asociadas:
	uint8_t _tablaRD[TAM_TABLA_RD][2];
	uint8_t _tamTablaRD;

// parámetros de configuración:
	uint8_t _id;
	uint8_t _incrementoTemporal;

// contadores:
	uint8_t _contSegundos;
	uint8_t _contAnuncio;

// objeto UDP
	EthernetUDP _UDP;
};

class nodoSensor{
public:
	// El constructor no tiene argumentos y prácticamente es inutil. Se deben usar las funciones de inicalización
	nodoSensor();
	void inicializarNS(uint8_t id, uint8_t incrementoTemporal, uint8_t* bandera, uint8_t* tam, uint8_t* datos);
	void actualizarNS();
	int getEstado();
	// métodos CEPER
	void procesarCEPER(mensaje Msg);
	void anunciar();
	void descubrir();
	int comprobarRA(uint8_t IDpasarela);
	int comprobarRD(uint8_t ID, uint8_t pasarela);
	void ack(mensaje Msg);
	void asociar(mensaje Msg);

	//métodos PER
	void procesarPER(mensaje Msg);
	void aceptarPER(mensaje Msg);
	void envioA(mensaje Msg);
	void reenvioA(mensaje Msg);
	void reenvioD(mensaje Msg); // Al no haber configuración este método tiene poco sentido

	//métodos PGD
	// Devuelve un puntero a una tabla reservada dinámicamente 
	// con un mensaje preparado para ser enviado que contiene las 
	// medidas. En tam pone el tamaño del mensaje. Si es 0, no hay 
	// medida preparada.
	uint8_t* medir(uint8_t* tam); //REVISAR

	// Método PGD
	// no hace falta en nodos sensores (solo emiten datos)
	//void procesar(pgdMsg* Msg, uint8_t origen);
private:

// puntero a variable de estado:	
	uint8_t _estado;

// tabalas de reenvío y variables asociadas:
	uint8_t _tablaRA[TAM_TABLA_RA*3];
	uint8_t _tamTablaRA;
	uint8_t _tablaRD[TAM_TABLA_RD][2];
	uint8_t _tamTablaRD;

// parámetros de configuración:
	uint8_t _id;
	uint8_t _incrementoTemporal;

// contadores:
	uint8_t _contSegundos;
	uint8_t _contAnuncio;
	uint8_t _contDescubrir;
	uint8_t _contMedicion;

// Interfaz para acceder a los datos medidos
	uint8_t* _bandera; // estará a 1 si los datos están listos para leerse
	uint8_t* _tam;
	uint8_t* _datos;
};
