// Practica tema 7, Velasco Gil Alvaro

// El servidor está situado en 10.0.25.250
// datos-cortos.dat = 100 Bytes
// datos-justos.dat = 512 Bytes
// datos-largos.dat = 12 345 678 Bytes

//  servidor UDP
//  tftp-client ip-servidor {-r|-w} archivo [-v]
//  Si se incluye un modo visual, hay que mostrar por consola
// todos los pasos de enviar y recibir paquetes y ACKs

// -----    LECTURA DEL FICHERO     -----
// datagrama de lectura:    RRQ
//                          01filename0octet0

// operation + filename0 + (octet)

// Los nombres de los ficheros, como máx 100 caracteres.
// El cliente solicita un fichero, el servidor comprueba si existe,
// lo abre, y va enviando bloques.
// Los paquetes tienen un numero de bloque, el primero es 1,
// y cada paquete es de hasta 512 bytes.
//          03numblockData
// Cuando el cliente recibe paquete, responde un ACK con el número de bloque.
//                                              04numblock
// Cuando el servidor recibe el ACK X, envía paquete X+1
// El último bloque tiene que ser menos a 512 bytes.
// El servidor puede mandar un código de error: 05errcodeErrstring0

// -----        ESCRITURA           -----
// datagrama de escritura:  WRQ
//                          02filename0octet0
// Si esta permitida la escritura, envía al cliente un ACK con el bloque 0.
// al recibirla el cliente, se pone a enviar paquetillos.
// Cada paquete, el servidor responde con un ACK y el numero del paquete,
// que al recibirlo el cliente, envia el siguiente paquete
