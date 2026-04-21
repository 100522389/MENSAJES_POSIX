/* clavesRPC.x - Interfaz ONC RPC para el servicio de claves */

/* Constantes */
const MAX_KEY    = 256;
const MAX_VALUE1 = 256;
const MAX_VECTOR = 32;

/* Tipos de datos */

struct paquete_t {
    int x;
    int y;
    int z;
};

/* Argumentos y resultados para cada operación */

/* destroy */
struct destroy_res {
    int status;
};

/* set_value */
struct set_args {
    string key<MAX_KEY>;
    string value1<MAX_VALUE1>;
    int    N_value2;
    float  V_value2<MAX_VECTOR>;
    paquete_t value3;
};

struct set_res {
    int status;
};

/* get_value */
struct get_args {
    string key<MAX_KEY>;
};

struct get_res {
    int       status;
    string    value1<MAX_VALUE1>;
    int       N_value2;
    float     V_value2<MAX_VECTOR>;
    paquete_t value3;
};

/* modify_value */
struct modify_args {
    string key<MAX_KEY>;
    string value1<MAX_VALUE1>;
    int    N_value2;
    float  V_value2<MAX_VECTOR>;
    paquete_t value3;
};

struct modify_res {
    int status;
};

/* delete_key */
struct delete_args {
    string key<MAX_KEY>;
};

struct delete_res {
    int status;
};

/* exist */
struct exist_args {
    string key<MAX_KEY>;
};

struct exist_res {
    int status;
};

/* Programa RPC - Serializer y conexión*/

program CLAVES_PROG {
    version CLAVES_VERS {
        destroy_res  RPC_DESTROY(void)           = 1;
        set_res      RPC_SET(set_args)           = 2;
        get_res      RPC_GET(get_args)           = 3;
        modify_res   RPC_MODIFY(modify_args)     = 4;
        delete_res   RPC_DELETE(delete_args)     = 5;
        exist_res    RPC_EXIST(exist_args)       = 6;
    } = 1;
} = 0x20001234;
