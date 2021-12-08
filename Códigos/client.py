#
#	Autor: Pablo Linares
#
#	Descripción: Ejemplo de cliente sencillo para la red WJSN. Escrito en python 3
#
#	Fecha: 15/1/2021
#

import time
import socket

HostIP = "169.254.1.177"
Npuerto = 5027
Mensaje = b"ASOCIAR"

puerto = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#puerto.bind(("169.254.141.235", 53048))
puerto.sendto(Mensaje, (HostIP, Npuerto))

while True:
	fichero = open("recibido.txt", "a+")
	data, addr = puerto.recvfrom(1024)
	fichero.write("Hora: ")
	fichero.write(time.ctime())
	fichero.write("; Recibido: ")
	fichero.write(str(list(data)))
	fichero.write("; origen: ")
	fichero.write(str(addr))
	fichero.write(";\n")
	fichero.close()
