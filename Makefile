CC = gcc
CFLAGS = -Wall -Wextra -pthread -I/usr/include/tirpc
LDFLAGS = -L. -Wl,-rpath=.
RPC_LIBS = -ltirpc -lpthread

# Archivos generados por rpcgen a partir de clavesRPC.x
RPC_GEN = clavesRPC.h clavesRPC_xdr.c clavesRPC_clnt.c clavesRPC_svc.c

all: libclaves.so libproxyclaves_sock.so libproxyclaves_mq.so libproxyclaves_rpc.so \
     servidor_sock servidor_mq servidor_rpc \
     cliente_sock cliente_queue cliente_mono cliente_rpc

# Targets por variante
mq:      libclaves.so libproxyclaves_mq.so servidor_mq cliente_queue
sockets: libclaves.so libproxyclaves_sock.so servidor_sock cliente_sock
rpc:     libclaves.so libproxyclaves_rpc.so servidor_rpc cliente_rpc

.PHONY: all mq sockets rpc clean

# Librería (Módulo de claves)
libclaves.so: claves.c claves.h
	$(CC) $(CFLAGS) -fPIC -shared -o libclaves.so claves.c

# Librería proxy por sockets (P2)
libproxyclaves_sock.so: proxy-sock.c claves.h common-mq.h
	$(CC) $(CFLAGS) -fPIC -shared -o libproxyclaves_sock.so proxy-sock.c

# Librería proxy por colas (P1)
libproxyclaves_mq.so: proxy-mq.c claves.h common-mq.h
	$(CC) $(CFLAGS) -fPIC -shared -o libproxyclaves_mq.so proxy-mq.c

# Servidor por sockets (P2)
servidor_sock: servidor-sock.c claves.h common-mq.h libclaves.so
	$(CC) $(CFLAGS) -o servidor_sock servidor-sock.c $(LDFLAGS) -lclaves

# Servidor de colas (P1)
servidor_mq: servidor-mq.c claves.h common-mq.h libclaves.so
	$(CC) $(CFLAGS) -o servidor_mq servidor-mq.c $(LDFLAGS) -lclaves

# Cliente principal (P2 - Enlazando con proxy socket)
cliente_sock: app-cliente.c claves.h libproxyclaves_sock.so
	$(CC) $(CFLAGS) -o cliente_sock app-cliente.c $(LDFLAGS) -lproxyclaves_sock

# Cliente con colas (P1 - Enlazando con proxy colas)
cliente_queue: app-cliente.c claves.h libproxyclaves_mq.so
	$(CC) $(CFLAGS) -o cliente_queue app-cliente.c $(LDFLAGS) -lproxyclaves_mq

# Cliente sin colas (Arquitectura monolítica original, de la fase A)
cliente_mono: app-cliente.c claves.h libclaves.so
	$(CC) $(CFLAGS) -o cliente_mono app-cliente.c $(LDFLAGS) -lclaves

# RPC: generación de stubs
# rpcgen genera: clavesRPC.h, clavesRPC_xdr.c, clavesRPC_clnt.c, clavesRPC_svc.c
$(RPC_GEN): clavesRPC.x
	rpcgen -N clavesRPC.x

# Librería proxy RPC (cliente)
libproxyclaves_rpc.so: proxy-rpc.c clavesRPC_xdr.c clavesRPC_clnt.c clavesRPC.h claves.h
	$(CC) $(CFLAGS) -fPIC -shared -o libproxyclaves_rpc.so \
	    proxy-rpc.c clavesRPC_xdr.c clavesRPC_clnt.c $(RPC_LIBS)

# Servidor RPC (une servidor-rpc.c + stub servidor + xdr + lógica de negocio)
servidor_rpc: servidor-rpc.c clavesRPC_svc.c clavesRPC_xdr.c clavesRPC.h libclaves.so
	$(CC) $(CFLAGS) -o servidor_rpc \
	    servidor-rpc.c clavesRPC_svc.c clavesRPC_xdr.c \
	    $(LDFLAGS) -lclaves $(RPC_LIBS)

# Cliente RPC (enlaza con proxy RPC)
cliente_rpc: app-cliente.c claves.h libproxyclaves_rpc.so
	$(CC) $(CFLAGS) -o cliente_rpc app-cliente.c $(LDFLAGS) -lproxyclaves_rpc $(RPC_LIBS)

clean:
	rm -f libclaves.so libproxyclaves_sock.so libproxyclaves_mq.so libproxyclaves_rpc.so \
	      servidor_sock servidor_mq servidor_rpc \
	      cliente_sock cliente_queue cliente_mono cliente_rpc \
	      $(RPC_GEN) clavesRPC_server.c clavesRPC_client.c
