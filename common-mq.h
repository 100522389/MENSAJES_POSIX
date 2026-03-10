#ifndef COMMON_MQ_H
#define COMMON_MQ_H

#include <mqueue.h>
#include "claves.h"

#define SERVER_QUEUE "/servidor_mq_req"
#define CLIENT_QUEUE_PREFIX "/cliente_mq_res_"
#define MAX_QUEUE_NAME 256

// Opcodes para las operaciones de la API
typedef enum {
    OP_INIT,
    OP_DESTROY,
    OP_SET,
    OP_GET,
    OP_MODIFY,
    OP_DELETE,
    OP_EXIST
} OperationCode;

// Estructura para peticiones (Cliente -> Servidor)
typedef struct {
    OperationCode opcode;
    char client_queue[MAX_QUEUE_NAME];
    
    // Parámetros de la API
    char key[256];
    char value1[256];
    int N_value2;
    float V_value2[32];
    struct Paquete value3;
} Request;

// Estructura para respuestas (Servidor -> Cliente)
typedef struct {
    int status; // Valor de retorno de la función (0, -1, 1, etc.)
    
    // Parámetros de salida (útiles para get_value)
    char value1[256];
    int N_value2;
    float V_value2[32];
    struct Paquete value3;
} Response;

#endif // COMMON_MQ_H
