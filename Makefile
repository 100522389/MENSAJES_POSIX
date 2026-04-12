CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -L. -Wl,-rpath=.

all: libclaves.so libproxyclaves.so libproxyclaves_mq.so servidor servidor_mq cliente cliente_queue cliente_mono

# A) Librería (Módulo de claves)
libclaves.so: claves.c claves.h
	$(CC) $(CFLAGS) -fPIC -shared -o libclaves.so claves.c

# B) Librería proxy por sockets (P2)
libproxyclaves.so: proxy-sock.c claves.h common-mq.h
	$(CC) $(CFLAGS) -fPIC -shared -o libproxyclaves.so proxy-sock.c

# B2) Librería proxy por colas (P1)
libproxyclaves_mq.so: proxy-mq.c claves.h common-mq.h
	$(CC) $(CFLAGS) -fPIC -shared -o libproxyclaves_mq.so proxy-mq.c

# Servidor por sockets (P2)
servidor: servidor-sock.c claves.h common-mq.h libclaves.so
	$(CC) $(CFLAGS) -o servidor servidor-sock.c $(LDFLAGS) -lclaves

# Servidor de colas (P1)
servidor_mq: servidor-mq.c claves.h common-mq.h libclaves.so
	$(CC) $(CFLAGS) -o servidor_mq servidor-mq.c $(LDFLAGS) -lclaves

# Cliente principal (P2 - Enlazando con proxy socket)
cliente: app-cliente.c claves.h libproxyclaves.so
	$(CC) $(CFLAGS) -o cliente app-cliente.c $(LDFLAGS) -lproxyclaves

# Cliente con colas (P1 - Enlazando con proxy colas)
cliente_queue: app-cliente.c claves.h libproxyclaves_mq.so
	$(CC) $(CFLAGS) -o cliente_queue app-cliente.c $(LDFLAGS) -lproxyclaves_mq

# Cliente sin colas (Arquitectura monolítica original, de la fase A)
cliente_mono: app-cliente.c claves.h libclaves.so
	$(CC) $(CFLAGS) -o cliente_mono app-cliente.c $(LDFLAGS) -lclaves

clean:
	rm -f libclaves.so libproxyclaves.so libproxyclaves_mq.so servidor servidor_mq cliente cliente_queue cliente_mono
