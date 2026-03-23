#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "claves.h"


static int check = 0;
static int failed = 0;

typedef struct {
    int worker_id;
    int iterations;
    char prefix[64];
    int failures;
} BatchWorkerArgs;

#define CHECK(desc, expr, expected) do { \
    int _r = (expr); \
    if (_r == (expected)) { \
        printf("  [OK] %s (ret=%d)\n", desc, _r); \
        check++; \
    } else { \
        printf("  [FAIL] %s: esperado %d, obtenido %d\n", desc, expected, _r); \
        failed++; \
    } \
} while (0)

static void print_paquete(struct Paquete p) {
    printf("       Paquete: x=%d y=%d z=%d\n", p.x, p.y, p.z);
}

static void build_key(char *dst, size_t dst_size, const char *prefix, const char *base) {
    if (prefix != NULL && prefix[0] != '\0') {
        snprintf(dst, dst_size, "%s_%s", prefix, base);
    } else {
        snprintf(dst, dst_size, "%s", base);
    }
    dst[dst_size - 1] = '\0';
}

static int run_concurrency_case(const char *prefix) {
    char key[256];
    char value1[256];
    int n2 = 0;
    float vec[32] = {0};
    float vecA[3] = {1.0f, 2.0f, 3.0f};
    float vecB[2] = {9.0f, 8.0f};
    struct Paquete pA = {1, 2, 3};
    struct Paquete pB = {7, 8, 9};
    struct Paquete pOut;

    build_key(key, sizeof(key), prefix, "cc_key");

    if (set_value(key, "v1", 3, vecA, pA) != 0) {
        return 1;
    }

    if (get_value(key, value1, &n2, vec, &pOut) != 0) {
        return 1;
    }
    if (strcmp(value1, "v1") != 0 || n2 != 3 || pOut.x != 1) {
        return 1;
    }

    if (modify_value(key, "v2", 2, vecB, pB) != 0) {
        return 1;
    }

    if (get_value(key, value1, &n2, vec, &pOut) != 0) {
        return 1;
    }
    if (strcmp(value1, "v2") != 0 || n2 != 2 || pOut.x != 7) {
        return 1;
    }

    if (delete_key(key) != 0) {
        return 1;
    }
    if (exist(key) != 0) {
        return 1;
    }

    return 0;
}

static void build_batch_key(char *dst, size_t dst_size, const char *prefix, int worker_id, int it) {
    if (prefix != NULL && prefix[0] != '\0') {
        snprintf(dst, dst_size, "%s_batch_w%d_i%d", prefix, worker_id, it);
    } else {
        snprintf(dst, dst_size, "batch_w%d_i%d", worker_id, it);
    }
    dst[dst_size - 1] = '\0';
}

static void *batch_worker(void *arg) {
    BatchWorkerArgs *w = (BatchWorkerArgs *)arg;
    float vecA[3] = {1.0f, 2.0f, 3.0f};
    float vecB[2] = {4.0f, 5.0f};
    struct Paquete pA = {10, 20, 30};
    struct Paquete pB = {40, 50, 60};

    for (int i = 0; i < w->iterations; i++) {
        char key[256];
        char value1[256] = {0};
        int n2 = 0;
        float vec[32] = {0};
        struct Paquete pOut = {0, 0, 0};

        build_batch_key(key, sizeof(key), w->prefix, w->worker_id, i);

        if (set_value(key, "batch_v1", 3, vecA, pA) != 0) {
            w->failures++;
            continue;
        }
        if (get_value(key, value1, &n2, vec, &pOut) != 0) {
            w->failures++;
            continue;
        }
        if (strcmp(value1, "batch_v1") != 0 || n2 != 3 || pOut.x != pA.x) {
            w->failures++;
            continue;
        }

        if (modify_value(key, "batch_v2", 2, vecB, pB) != 0) {
            w->failures++;
            continue;
        }
        if (get_value(key, value1, &n2, vec, &pOut) != 0) {
            w->failures++;
            continue;
        }
        if (strcmp(value1, "batch_v2") != 0 || n2 != 2 || pOut.x != pB.x) {
            w->failures++;
            continue;
        }

        if (delete_key(key) != 0) {
            w->failures++;
            continue;
        }
        if (exist(key) != 0) {
            w->failures++;
            continue;
        }
    }

    return NULL;
}

static int run_batch_case(const char *prefix, int threads, int iterations) {
    pthread_t *ids;
    BatchWorkerArgs *args;
    int total_failures = 0;

    if (threads <= 0 || iterations <= 0) {
        return 1;
    }

    ids = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)threads);
    args = (BatchWorkerArgs *)calloc((size_t)threads, sizeof(BatchWorkerArgs));
    if (ids == NULL || args == NULL) {
        free(ids);
        free(args);
        return 1;
    }

    for (int i = 0; i < threads; i++) {
        args[i].worker_id = i;
        args[i].iterations = iterations;
        args[i].failures = 0;
        if (prefix != NULL) {
            strncpy(args[i].prefix, prefix, sizeof(args[i].prefix) - 1);
            args[i].prefix[sizeof(args[i].prefix) - 1] = '\0';
        }
        if (pthread_create(&ids[i], NULL, batch_worker, &args[i]) != 0) {
            args[i].failures++;
            total_failures++;
            ids[i] = 0;
        }
    }

    for (int i = 0; i < threads; i++) {
        if (ids[i] != 0) {
            (void)pthread_join(ids[i], NULL);
        }
        total_failures += args[i].failures;
    }

    free(ids);
    free(args);

    return total_failures == 0 ? 0 : 1;
}


int main(void) {
    char    v1[256];
    int     n2;
    float   vec[32];
    struct  Paquete p;

    /* Datos de prueba */
    struct Paquete pA = {1, 2, 3};
    struct Paquete pB = {10, 20, 30};
    float  vecA[6] = {1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f};
    float  vecB[2] = {9.9f, 8.8f};
    const char *concurrency_mode = getenv("TEST_CONCURRENCY");
    const char *batch_mode = getenv("TEST_CONCURRENCY_BATCH");
    const char *batch_threads_env = getenv("TEST_BATCH_THREADS");
    const char *batch_iters_env = getenv("TEST_BATCH_ITERATIONS");
    const char *test_prefix = getenv("TEST_KEY_PREFIX");
    int batch_threads = 8;
    int batch_iterations = 100;
    char keyA[256], keyB[256], keyX[256], keyN1[256], keyN32[256];

    if (batch_threads_env != NULL) {
        int parsed = atoi(batch_threads_env);
        if (parsed > 0) {
            batch_threads = parsed;
        }
    }
    if (batch_iters_env != NULL) {
        int parsed = atoi(batch_iters_env);
        if (parsed > 0) {
            batch_iterations = parsed;
        }
    }

    build_key(keyA, sizeof(keyA), test_prefix, "claveA");
    build_key(keyB, sizeof(keyB), test_prefix, "claveB");
    build_key(keyX, sizeof(keyX), test_prefix, "claveX");
    build_key(keyN1, sizeof(keyN1), test_prefix, "kN1");
    build_key(keyN32, sizeof(keyN32), test_prefix, "kN32");

    if (concurrency_mode != NULL && strcmp(concurrency_mode, "1") == 0) {
        return run_concurrency_case(test_prefix);
    }
    if (batch_mode != NULL && strcmp(batch_mode, "1") == 0) {
        return run_batch_case(test_prefix, batch_threads, batch_iterations);
    }

    printf("Plan de pruebas: libclaves\n\n");

    printf("-- destroy sobre tabla vacía --\n");
    CHECK("destroy vacío", destroy(), 0);

    printf("\n-- set_value: inserción básica --\n");
    CHECK("set_value claveA", set_value(keyA, "valor1A", 4, vecA, pA), 0);
    CHECK("set_value claveB", set_value(keyB, "valor1B", 2, vecB, pB), 0);

    printf("\n-- set_value: clave duplicada --\n");
    CHECK("set_value duplicado claveA", set_value(keyA, "otro", 1, vecA, pA), -1);

    printf("\n-- set_value: parámetros inválidos --\n");
    CHECK("set_value key=NULL",    set_value(NULL,     "v", 1, vecA, pA), -1);
    CHECK("set_value value1=NULL", set_value("k",      NULL, 1, vecA, pA), -1);
    CHECK("set_value vec=NULL",    set_value("k",      "v", 1, NULL, pA), -1);
    CHECK("set_value N=0",         set_value("k",      "v", 0, vecA, pA), -1);
    CHECK("set_value N=33",        set_value("k",      "v", 33, vecA, pA), -1);

    printf("\n-- 5. exist --\n");
    CHECK("exist claveA (existe)",     exist(keyA), 1);
    CHECK("exist claveB (existe)",     exist(keyB), 1);
    CHECK("exist claveX (no existe)",  exist(keyX), 0);
    CHECK("exist NULL",                exist(NULL),    -1);

    printf("\n-- get_value: lectura --\n");
    memset(v1, 0, sizeof(v1));
    memset(vec, 0, sizeof(vec));
    if (get_value(keyA, v1, &n2, vec, &p) == 0) {
        printf("  [OK] get_value claveA\n");
        printf("       value1=\"%s\" N=%d\n", v1, n2);
        printf("       vec[0]=%.2f vec[1]=%.2f vec[2]=%.2f vec[3]=%.2f\n",
               vec[0], vec[1], vec[2], vec[3]);
        print_paquete(p);
        /* Validar contenido */
        if (strcmp(v1, "valor1A") == 0 && n2 == 4 &&
            vec[0] == vecA[0] && p.x == pA.x && p.y == pA.y && p.z == pA.z) {
            printf("  [OK] contenido claveA verificado\n");
            check++;
        } else {
            printf("  [FAIL] contenido claveA incorrecto\n");
            failed++;
        }
        check++;
    } else {
        printf("  [FAIL] get_value claveA devolvió -1\n");
        failed++;
    }

    printf("\n-- get_value: clave inexistente --\n");
    CHECK("get_value claveX", get_value(keyX, v1, &n2, vec, &p), -1);

    printf("\n-- get_value: parámetros NULL --\n");
    CHECK("get_value key=NULL",    get_value(NULL,     v1, &n2, vec, &p), -1);
    CHECK("get_value value1=NULL", get_value(keyA, NULL, &n2, vec, &p), -1);
    CHECK("get_value N=NULL",      get_value(keyA, v1, NULL, vec, &p), -1);
    CHECK("get_value vec=NULL",    get_value(keyA, v1, &n2, NULL, &p), -1);
    CHECK("get_value p=NULL",      get_value(keyA, v1, &n2, vec, NULL), -1);

    printf("\n-- modify_value --\n");
    struct Paquete pMod = {99, 88, 77};
    float vecMod[3] = {5.5f, 6.6f, 7.7f};
    CHECK("modify_value claveA", modify_value(keyA, "nuevoValor1", 3, vecMod, pMod), 0);

    memset(v1, 0, sizeof(v1));
    memset(vec, 0, sizeof(vec));
    if (get_value(keyA, v1, &n2, vec, &p) == 0) {
        if (strcmp(v1, "nuevoValor1") == 0 && n2 == 3 &&
            vec[0] == vecMod[0] && p.x == 99) {
            printf("  [OK] contenido modificado verificado\n");
            check++;
        } else {
            printf("  [FAIL] contenido tras modify_value\n");
            failed++;
        }
    }

    printf("\n--  modify_value: clave inexistente --\n");
    CHECK("modify_value claveX", modify_value(keyX, "v", 1, vecA, pA), -1);

    printf("\n--  modify_value: parámetros inválidos --\n");
    CHECK("modify_value N=0",  modify_value(keyA, "v", 0,  vecA, pA), -1);
    CHECK("modify_value N=33", modify_value(keyA, "v", 33, vecA, pA), -1);

    printf("\n--  delete_key --\n");
    CHECK("delete_key claveB",          delete_key(keyB),  0);
    CHECK("exist claveB tras borrar",   exist(keyB),        0);
    CHECK("delete_key claveB 2ª vez",   delete_key(keyB), -1);
    CHECK("delete_key NULL",            delete_key(NULL),      -1);

    printf("\n--  destroy con elementos --\n");
    CHECK("destroy con claveA", destroy(), 0);
    CHECK("exist claveA tras destroy", exist(keyA), 0);

    printf("\n-- destroy doble --\n");
    CHECK("destroy vacío 2ª vez", destroy(), 0);

    printf("\n-- inserción de key de 255 chars --\n");
    char key255[256];
    memset(key255, 'k', 255);
    key255[255] = '\0';
    if (test_prefix != NULL && test_prefix[0] != '\0') {
        size_t prefix_len = strlen(test_prefix);
        if (prefix_len > 200) {
            prefix_len = 200;
        }
        memcpy(key255, test_prefix, prefix_len);
        if (prefix_len < 255) {
            key255[prefix_len] = '_';
        }
    }
    CHECK("set_value key 255 chars", set_value(key255, "v", 1, vecA, pA), 0);
    CHECK("exist key 255 chars",     exist(key255), 1);
    CHECK("delete_key key 255 chars", delete_key(key255), 0);

    printf("\n-- inserción de N_value2 en límites (1 y 32) --\n");
    float vecLimite[32];
    for (int i = 0; i < 32; i++) vecLimite[i] = (float)i;
    CHECK("set_value N=1",  set_value(keyN1,  "v", 1,  vecLimite, pA), 0);
    CHECK("set_value N=32", set_value(keyN32, "v", 32, vecLimite, pA), 0);
    destroy();

        printf("\n-- Peticiones en paralelo para el servidor --\n");
        CHECK("batch paralelo (8 hilos x 100 peticiones)",
            run_batch_case(test_prefix, batch_threads, batch_iterations),
            0);

    printf("\n");
    printf("  Aciertos: %d  |  X: %d  |  Total: %d\n",
           check, failed, check + failed);
    printf("\n");

    return failed == 0 ? 0 : 1;
}
