#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "claves.h"


static int check = 0;
static int failed = 0;

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

    printf("Plan de pruebas: libclaves\n\n");

    printf("-- 1. destroy sobre tabla vacía --\n");
    CHECK("destroy vacío", destroy(), 0);

    printf("\n-- 2. set_value: inserción básica --\n");
    CHECK("set_value claveA", set_value("claveA", "valor1A", 4, vecA, pA), 0);
    CHECK("set_value claveB", set_value("claveB", "valor1B", 2, vecB, pB), 0);

    printf("\n-- 3. set_value: clave duplicada --\n");
    CHECK("set_value duplicado claveA", set_value("claveA", "otro", 1, vecA, pA), -1);

    printf("\n-- 4. set_value: parámetros inválidos --\n");
    CHECK("set_value key=NULL",    set_value(NULL,     "v", 1, vecA, pA), -1);
    CHECK("set_value value1=NULL", set_value("k",      NULL, 1, vecA, pA), -1);
    CHECK("set_value vec=NULL",    set_value("k",      "v", 1, NULL, pA), -1);
    CHECK("set_value N=0",         set_value("k",      "v", 0, vecA, pA), -1);
    CHECK("set_value N=33",        set_value("k",      "v", 33, vecA, pA), -1);

    printf("\n-- 5. exist --\n");
    CHECK("exist claveA (existe)",     exist("claveA"), 1);
    CHECK("exist claveB (existe)",     exist("claveB"), 1);
    CHECK("exist claveX (no existe)",  exist("claveX"), 0);
    CHECK("exist NULL",                exist(NULL),    -1);

    printf("\n-- 6. get_value: lectura --\n");
    memset(v1, 0, sizeof(v1));
    memset(vec, 0, sizeof(vec));
    if (get_value("claveA", v1, &n2, vec, &p) == 0) {
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

    printf("\n-- 7. get_value: clave inexistente --\n");
    CHECK("get_value claveX", get_value("claveX", v1, &n2, vec, &p), -1);

    printf("\n-- 8. get_value: parámetros NULL --\n");
    CHECK("get_value key=NULL",    get_value(NULL,     v1, &n2, vec, &p), -1);
    CHECK("get_value value1=NULL", get_value("claveA", NULL, &n2, vec, &p), -1);
    CHECK("get_value N=NULL",      get_value("claveA", v1, NULL, vec, &p), -1);
    CHECK("get_value vec=NULL",    get_value("claveA", v1, &n2, NULL, &p), -1);
    CHECK("get_value p=NULL",      get_value("claveA", v1, &n2, vec, NULL), -1);

    printf("\n-- 9. modify_value --\n");
    struct Paquete pMod = {99, 88, 77};
    float vecMod[3] = {5.5f, 6.6f, 7.7f};
    CHECK("modify_value claveA", modify_value("claveA", "nuevoValor1", 3, vecMod, pMod), 0);

    memset(v1, 0, sizeof(v1));
    memset(vec, 0, sizeof(vec));
    if (get_value("claveA", v1, &n2, vec, &p) == 0) {
        if (strcmp(v1, "nuevoValor1") == 0 && n2 == 3 &&
            vec[0] == vecMod[0] && p.x == 99) {
            printf("  [OK] contenido modificado verificado\n");
            check++;
        } else {
            printf("  [FAIL] contenido tras modify_value\n");
            failed++;
        }
    }

    printf("\n-- 10. modify_value: clave inexistente --\n");
    CHECK("modify_value claveX", modify_value("claveX", "v", 1, vecA, pA), -1);

    printf("\n-- 11. modify_value: parámetros inválidos --\n");
    CHECK("modify_value N=0",  modify_value("claveA", "v", 0,  vecA, pA), -1);
    CHECK("modify_value N=33", modify_value("claveA", "v", 33, vecA, pA), -1);

    printf("\n-- 12. delete_key --\n");
    CHECK("delete_key claveB",          delete_key("claveB"),  0);
    CHECK("exist claveB tras borrar",   exist("claveB"),        0);
    CHECK("delete_key claveB 2ª vez",   delete_key("claveB"), -1);
    CHECK("delete_key NULL",            delete_key(NULL),      -1);

    printf("\n-- 13. destroy con elementos --\n");
    CHECK("destroy con claveA", destroy(), 0);
    CHECK("exist claveA tras destroy", exist("claveA"), 0);

    printf("\n-- 14. destroy doble --\n");
    CHECK("destroy vacío 2ª vez", destroy(), 0);

    printf("\n-- 15. inserción de key de 255 chars --\n");
    char key255[256];
    memset(key255, 'k', 255);
    key255[255] = '\0';
    CHECK("set_value key 255 chars", set_value(key255, "v", 1, vecA, pA), 0);
    CHECK("exist key 255 chars",     exist(key255), 1);
    CHECK("delete_key key 255 chars", delete_key(key255), 0);

    printf("\n-- 16. inserción de N_value2 en límites (1 y 32) --\n");
    float vecLimite[32];
    for (int i = 0; i < 32; i++) vecLimite[i] = (float)i;
    CHECK("set_value N=1",  set_value("kN1",  "v", 1,  vecLimite, pA), 0);
    CHECK("set_value N=32", set_value("kN32", "v", 32, vecLimite, pA), 0);
    destroy();

    printf("\n");
    printf("  Aciertos: %d  |  X: %d  |  Total: %d\n",
           check, failed, check + failed);
    printf("\n");

    return failed == 0 ? 0 : 1;
}
