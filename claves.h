#ifndef CLAVES_H
#define CLAVES_H


struct Paquete {
   int x ;
   int y ;
   int z ;
} ;


int destroy(void);

/**
 * @param key clave.
 * @param value1   valor1 [256].
 * @param N_value2 dimensión del vector V_value2 [1-32].
 * @param V_value2 vector de floats [32].
 * @param value3   estructura Paquete.
 * @return int El servicio devuelve 0 si se insertó con éxito y -1 en caso de error.
 * @retval 0 si se insertó con éxito.
 * @retval -1.
 */
int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3);

/**
 * @param key clave.
 * @param value1   valor1 [256].
 * @param N_value2 dimensión del vector V_value2 [1-32] por referencia.
 * @param V_value2 vector de floats [32].
 * @param value3   estructura Paquete por referencia.
 * @return int El servicio devuelve 0 si se insertó con éxito y -1 en caso de error.
 * @retval 0 en caso de éxito.
 * @retval -1.
 */
int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3);

/**
 * @param key clave.
 * @param value1   valor1 [256].
 * @param N_value2 dimensión del vector V_value2 [1-32].
 * @param V_value2 vector de floats [32].
 * @param value3   estructura Paquete.
 * @return int El servicio devuelve 0 si se insertó con éxito y -1 en caso de error.
 * @retval 0 si se modificó con éxito.
 * @retval -1.
 */
int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3);

/**
 * @param key clave.
 * @return int La función devuelve 0 en caso de éxito y -1 (Excepciones)
 * @retval 0 en caso de éxito.
 * @retval -1.
 */
int delete_key(char *key);

/**
 * @param key clave.
 * @return int La función devuelve 1 en caso de que exista y 0 en caso de que no exista. -1 (Excepciones).
 * @retval 1 en caso de que exista.
 * @retval 0 en caso de que no exista.
 * @retval -1.
 */
int exist(char *key);

#endif
