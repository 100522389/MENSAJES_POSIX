CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -L. -Wl,-rpath=.

all: libclaves.so libproxyclaves.so libproxyclaves_sock.so servidor_mq servidor_sock cliente cliente_queue cliente_sock

# A) Librería (Módulo de claves)
libclaves.so: claves.c claves.h
	$(CC) $(CFLAGS) -fPIC -shared -o libclaves.so claves.c

# B) Librería proxy (lado cliente)
libproxyclaves.so: proxy-mq.c claves.h common-mq.h
	$(CC) $(CFLAGS) -fPIC -shared -o libproxyclaves.so proxy-mq.c

# B2) Librería proxy por sockets (lado cliente)
libproxyclaves_sock.so: proxy-sock.c claves.h common-mq.h
	$(CC) $(CFLAGS) -fPIC -shared -o libproxyclaves_sock.so proxy-sock.c

# Servidor de colas
servidor_mq: servidor-mq.c claves.h common-mq.h libclaves.so
	$(CC) $(CFLAGS) -o servidor_mq servidor-mq.c $(LDFLAGS) -lclaves

# Servidor por sockets
servidor_sock: servidor-sock.c claves.h common-mq.h libclaves.so
	$(CC) $(CFLAGS) -o servidor_sock servidor-sock.c $(LDFLAGS) -lclaves

# Cliente sin colas (Arquitectura monolítica original, de la fase A)
cliente: cliente.c claves.h libclaves.so
	$(CC) $(CFLAGS) -o cliente cliente.c $(LDFLAGS) -lclaves

# Cliente con colas (Distribuido, misma interfaz, enlazando con proxy proxyclaves)
cliente_queue: cliente.c claves.h libproxyclaves.so
	$(CC) $(CFLAGS) -o cliente_queue cliente.c $(LDFLAGS) -lproxyclaves

# Cliente con sockets (Distribuido, misma interfaz, enlazando con proxy socket)
cliente_sock: cliente.c claves.h libproxyclaves_sock.so
	$(CC) $(CFLAGS) -o cliente_sock cliente.c $(LDFLAGS) -lproxyclaves_sock

clean:
	rm -f libclaves.so libproxyclaves.so libproxyclaves_sock.so servidor_mq servidor_sock cliente cliente_queue cliente_sock
