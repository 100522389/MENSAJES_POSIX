#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc/rpc.h>
#include "clavesRPC.h"   /* generado por rpcgen */
#include "claves.h"

/* destroy */
destroy_res *
rpc_destroy_1_svc(struct svc_req *rqstp)
{
    (void)rqstp;
    static destroy_res result;
    result.status = destroy();
    return &result;
}

/* set_value */
set_res *
rpc_set_1_svc(set_args argp, struct svc_req *rqstp)
{
    (void)rqstp;
    static set_res result;

    struct Paquete p;
    p.x = argp.value3.x;
    p.y = argp.value3.y;
    p.z = argp.value3.z;

    result.status = set_value(
        argp.key,
        argp.value1,
        argp.N_value2,
        argp.V_value2.V_value2_val,
        p
    );
    return &result;
}

/* get_value */
get_res *
rpc_get_1_svc(get_args argp, struct svc_req *rqstp)
{
    (void)rqstp;
    static get_res result;
    static char value1_buf[256];
    static float vec_buf[32];
    static struct Paquete pkt;

    /* Liberar cadenas XDR de llamadas anteriores */
    if (result.value1) {
        free(result.value1);
        result.value1 = NULL;
    }
    if (result.V_value2.V_value2_val) {
        free(result.V_value2.V_value2_val);
        result.V_value2.V_value2_val = NULL;
    }

    int N = 0;
    memset(value1_buf, 0, sizeof(value1_buf));
    memset(vec_buf, 0, sizeof(vec_buf));

    result.status = get_value(argp.key, value1_buf, &N, vec_buf, &pkt);

    result.value1 = strdup(value1_buf);
    result.N_value2 = N;

    result.V_value2.V_value2_val = malloc(N * sizeof(float));
    result.V_value2.V_value2_len = (N > 0) ? N : 0;
    if (N > 0 && result.V_value2.V_value2_val)
        memcpy(result.V_value2.V_value2_val, vec_buf, N * sizeof(float));

    result.value3.x = pkt.x;
    result.value3.y = pkt.y;
    result.value3.z = pkt.z;

    return &result;
}

/* modify_value */
modify_res *
rpc_modify_1_svc(modify_args argp, struct svc_req *rqstp)
{
    (void)rqstp;
    static modify_res result;

    struct Paquete p;
    p.x = argp.value3.x;
    p.y = argp.value3.y;
    p.z = argp.value3.z;

    result.status = modify_value(
        argp.key,
        argp.value1,
        argp.N_value2,
        argp.V_value2.V_value2_val,
        p
    );
    return &result;
}

/* delete_key */
delete_res *
rpc_delete_1_svc(delete_args argp, struct svc_req *rqstp)
{
    (void)rqstp;
    static delete_res result;
    result.status = delete_key(argp.key);
    return &result;
}

/* exist */
exist_res *
rpc_exist_1_svc(exist_args argp, struct svc_req *rqstp)
{
    (void)rqstp;
    static exist_res result;
    result.status = exist(argp.key);
    return &result;
}
