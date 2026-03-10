#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "claves.h"

typedef struct Nodo {
    char key[256];
    char value1[256];
    int N_value2;
    float *V_value2;
    struct Paquete value3;
    struct Nodo *next;
} Nodo;

static Nodo *head = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int destroy(void) {
    pthread_mutex_lock(&mutex);
    Nodo *actual = head;
    while (actual != NULL) {
        Nodo *next = actual->next;
        free(actual->V_value2);
        free(actual);
        actual = next;
    }
    head = NULL;
    pthread_mutex_unlock(&mutex);
    return 0;
}

int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    if (key == NULL || value1 == NULL || V_value2 == NULL)
        return -1;
    if (N_value2 < 1 || N_value2 > 32)
        return -1;
    pthread_mutex_lock(&mutex);
    Nodo *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        actual = actual->next;
    }
    Nodo *nuevoNodo = malloc(sizeof(Nodo));
    if (nuevoNodo == NULL) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    nuevoNodo->V_value2 = malloc(N_value2 * sizeof(float));
    if (nuevoNodo->V_value2 == NULL) {
        free(nuevoNodo);
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    strncpy(nuevoNodo->key,    key,    255); nuevoNodo->key[255] = '\0';
    strncpy(nuevoNodo->value1, value1, 255); nuevoNodo->value1[255] = '\0';
    nuevoNodo->N_value2 = N_value2;
    memcpy(nuevoNodo->V_value2, V_value2, N_value2 * sizeof(float));
    nuevoNodo->value3 = value3;
    nuevoNodo->next = head;
    head = nuevoNodo;
    pthread_mutex_unlock(&mutex);
    return 0;
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    if (key == NULL || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL)
        return -1;
    pthread_mutex_lock(&mutex);
    Nodo *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            strncpy(value1, actual->value1, 255); value1[255] = '\0';
            *N_value2 = actual->N_value2;
            memcpy(V_value2, actual->V_value2, actual->N_value2 * sizeof(float));
            *value3 = actual->value3;
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        actual = actual->next;
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    if (key == NULL || value1 == NULL || V_value2 == NULL)
        return -1;
    if (N_value2 < 1 || N_value2 > 32)
        return -1;
    pthread_mutex_lock(&mutex);
    Nodo *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            float *tmp = realloc(actual->V_value2, N_value2 * sizeof(float));
            if (tmp == NULL) {
                pthread_mutex_unlock(&mutex);
                return -1;
            }
            actual->V_value2 = tmp;
            strncpy(actual->value1, value1, 255); actual->value1[255] = '\0';
            actual->N_value2 = N_value2;
            memcpy(actual->V_value2, V_value2, N_value2 * sizeof(float));
            actual->value3 = value3;
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        actual = actual->next;
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}

int delete_key(char *key) {
    if (key == NULL)
        return -1;
    pthread_mutex_lock(&mutex);
    Nodo *actual = head;
    Nodo *prev = NULL;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            if (prev == NULL)
                head = actual->next;
            else
                prev->next = actual->next;
            free(actual->V_value2);
            free(actual);
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        prev = actual;
        actual = actual->next;
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}

int exist(char *key) {
    if (key == NULL)
        return -1;
    pthread_mutex_lock(&mutex);
    Nodo *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            pthread_mutex_unlock(&mutex);
            return 1;
        }
        actual = actual->next;
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}
