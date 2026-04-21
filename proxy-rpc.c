#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <rpc/rpc.h>
#include "clavesRPC.h"
#include "claves.h"

#define ENV_IP "IP_TUPLAS"

static pthread_mutex_t rpc_mutex = PTHREAD_MUTEX_INITIALIZER;

/*Obtener cliente RPC*/
static CLIENT *get_client(void)
{
    const char *host = getenv(ENV_IP);
    if (host == NULL || host[0] == '\0') {
        fprintf(stderr, "proxy-rpc: variable de entorno %s no definida\n", ENV_IP);
        return NULL;
    }
    CLIENT *clnt = clnt_create(host, CLAVES_PROG, CLAVES_VERS, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        return NULL;
    }
    return clnt;
}

/* destroy */
int destroy(void)
{
    CLIENT *clnt = get_client();
    if (clnt == NULL) return -1;

    pthread_mutex_lock(&rpc_mutex);
    destroy_res *res = rpc_destroy_1(clnt);
    if (res == NULL) {
        pthread_mutex_unlock(&rpc_mutex);
        clnt_perror(clnt, "rpc_destroy_1");
        clnt_destroy(clnt);
        return -1;
    }
    int ret = res->status;
    pthread_mutex_unlock(&rpc_mutex);
    clnt_destroy(clnt);
    return ret;
}

/* set_value */
int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3)
{
    if (key == NULL || value1 == NULL || V_value2 == NULL) return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    CLIENT *clnt = get_client();
    if (clnt == NULL) return -1;

    set_args args;
    args.key    = key;
    args.value1 = value1;
    args.N_value2 = N_value2;
    args.V_value2.V_value2_val = V_value2;
    args.V_value2.V_value2_len = N_value2;
    args.value3.x = value3.x;
    args.value3.y = value3.y;
    args.value3.z = value3.z;

    pthread_mutex_lock(&rpc_mutex);
    set_res *res = rpc_set_1(args, clnt);
    if (res == NULL) {
        pthread_mutex_unlock(&rpc_mutex);
        clnt_perror(clnt, "rpc_set_1");
        clnt_destroy(clnt);
        return -1;
    }
    int ret = res->status;
    pthread_mutex_unlock(&rpc_mutex);
    clnt_destroy(clnt);
    return ret;
}

/* get_value */
int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3)
{
    if (key == NULL || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL)
        return -1;

    CLIENT *clnt = get_client();
    if (clnt == NULL) return -1;

    get_args args;
    args.key = key;

    pthread_mutex_lock(&rpc_mutex);
    get_res *res = rpc_get_1(args, clnt);
    if (res == NULL) {
        pthread_mutex_unlock(&rpc_mutex);
        clnt_perror(clnt, "rpc_get_1");
        clnt_destroy(clnt);
        return -1;
    }

    int ret = res->status;
    if (ret == 0) {
        strncpy(value1, res->value1, 255);
        value1[255] = '\0';
        *N_value2 = res->N_value2;
        int n = res->N_value2;
        if (n > 32) n = 32;
        if (n > 0 && res->V_value2.V_value2_val)
            memcpy(V_value2, res->V_value2.V_value2_val, n * sizeof(float));
        value3->x = res->value3.x;
        value3->y = res->value3.y;
        value3->z = res->value3.z;
    }
    pthread_mutex_unlock(&rpc_mutex);

    clnt_destroy(clnt);
    return ret;
}

/* modify_value */
int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3)
{
    if (key == NULL || value1 == NULL || V_value2 == NULL) return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    CLIENT *clnt = get_client();
    if (clnt == NULL) return -1;

    modify_args args;
    args.key    = key;
    args.value1 = value1;
    args.N_value2 = N_value2;
    args.V_value2.V_value2_val = V_value2;
    args.V_value2.V_value2_len = N_value2;
    args.value3.x = value3.x;
    args.value3.y = value3.y;
    args.value3.z = value3.z;

    pthread_mutex_lock(&rpc_mutex);
    modify_res *res = rpc_modify_1(args, clnt);
    if (res == NULL) {
        pthread_mutex_unlock(&rpc_mutex);
        clnt_perror(clnt, "rpc_modify_1");
        clnt_destroy(clnt);
        return -1;
    }
    int ret = res->status;
    pthread_mutex_unlock(&rpc_mutex);
    clnt_destroy(clnt);
    return ret;
}

/* delete_key */
int delete_key(char *key)
{
    if (key == NULL) return -1;

    CLIENT *clnt = get_client();
    if (clnt == NULL) return -1;

    delete_args args;
    args.key = key;

    pthread_mutex_lock(&rpc_mutex);
    delete_res *res = rpc_delete_1(args, clnt);
    if (res == NULL) {
        pthread_mutex_unlock(&rpc_mutex);
        clnt_perror(clnt, "rpc_delete_1");
        clnt_destroy(clnt);
        return -1;
    }
    int ret = res->status;
    pthread_mutex_unlock(&rpc_mutex);
    clnt_destroy(clnt);
    return ret;
}

/* exist */
int exist(char *key)
{
    if (key == NULL) return -1;

    CLIENT *clnt = get_client();
    if (clnt == NULL) return -1;

    exist_args args;
    args.key = key;

    pthread_mutex_lock(&rpc_mutex);
    exist_res *res = rpc_exist_1(args, clnt);
    if (res == NULL) {
        pthread_mutex_unlock(&rpc_mutex);
        clnt_perror(clnt, "rpc_exist_1");
        clnt_destroy(clnt);
        return -1;
    }
    int ret = res->status;
    pthread_mutex_unlock(&rpc_mutex);
    clnt_destroy(clnt);
    return ret;
}
