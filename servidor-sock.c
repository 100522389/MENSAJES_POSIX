#include "claves.h"
#include "common-mq.h"
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int server_fd = -1;

#define KEY_MAX 255
#define VALUE1_MAX 255
#define MAX_VECTOR 32
int counter = 0;

typedef struct {
  OperationCode opcode;
  char key[256];
  char value1[256];
  int N_value2;
  float V_value2[MAX_VECTOR];
  struct Paquete value3;
} SockRequest;

typedef struct {
  int status;
  char value1[256];
  int N_value2;
  float V_value2[MAX_VECTOR];
  struct Paquete value3;
} SockResponse;

static int send_all(int fd, const void *buf, size_t len) {
  const char *p = (const char *)buf;
  size_t sent = 0;
  while (sent < len) {
    ssize_t n = send(fd, p + sent, len - sent, 0);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (n == 0)
      return -1;
    sent += (size_t)n;
  }
  return 0;
}

static int recv_all(int fd, void *buf, size_t len) {
  char *p = (char *)buf;
  size_t recvd = 0;
  while (recvd < len) {
    ssize_t n = recv(fd, p + recvd, len - recvd, 0);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (n == 0)
      return -1;
    recvd += (size_t)n;
  }
  return 0;
}

static int send_u32(int fd, uint32_t value) {
  uint32_t net = htonl(value);
  return send_all(fd, &net, sizeof(net));
}

static int recv_u32(int fd, uint32_t *value) {
  uint32_t net;
  if (recv_all(fd, &net, sizeof(net)) != 0)
    return -1;
  *value = ntohl(net);
  return 0;
}

static int send_i32(int fd, int value) { return send_u32(fd, (uint32_t)value); }

static int recv_i32(int fd, int *value) {
  uint32_t tmp;
  if (recv_u32(fd, &tmp) != 0)
    return -1;
  *value = (int)tmp;
  return 0;
}

static int send_float32(int fd, float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  return send_u32(fd, bits);
}

static int recv_float32(int fd, float *value) {
  uint32_t bits;
  if (recv_u32(fd, &bits) != 0)
    return -1;
  memcpy(value, &bits, sizeof(bits));
  return 0;
}

static int send_string(int fd, const char *s, size_t max_len) {
  uint32_t len = (uint32_t)strlen(s);
  if ((size_t)len > max_len)
    return -1;
  if (send_u32(fd, len) != 0)
    return -1;
  if (len == 0)
    return 0;
  return send_all(fd, s, len);
}

static int recv_string(int fd, char *dst, size_t dst_size, size_t max_len) {
  uint32_t len;
  if (recv_u32(fd, &len) != 0)
    return -1;
  if ((size_t)len > max_len || (size_t)len >= dst_size)
    return -1;
  if (len > 0 && recv_all(fd, dst, len) != 0)
    return -1;
  dst[len] = '\0';
  return 0;
}

static int send_float_array(int fd, const float *arr, int n) {
  int i;
  if (send_i32(fd, n) != 0)
    return -1;
  if (n < 0 || n > MAX_VECTOR)
    return -1;
  for (i = 0; i < n; i++) {
    if (send_float32(fd, arr[i]) != 0)
      return -1;
  }
  return 0;
}

static int recv_float_array(int fd, float *arr, int *n_out) {
  int i;
  int n;
  if (recv_i32(fd, &n) != 0)
    return -1;
  if (n < 0 || n > MAX_VECTOR)
    return -1;
  for (i = 0; i < n; i++) {
    if (recv_float32(fd, &arr[i]) != 0)
      return -1;
  }
  *n_out = n;
  return 0;
}

static int send_paquete(int fd, const struct Paquete *p) {
  if (send_i32(fd, p->x) != 0)
    return -1;
  if (send_i32(fd, p->y) != 0)
    return -1;
  return send_i32(fd, p->z);
}

static int recv_paquete(int fd, struct Paquete *p) {
  if (recv_i32(fd, &p->x) != 0)
    return -1;
  if (recv_i32(fd, &p->y) != 0)
    return -1;
  return recv_i32(fd, &p->z);
}

static int recv_request_wire(int fd, SockRequest *req) {
  uint32_t op;
  if (recv_u32(fd, &op) != 0)
    return -1;
  req->opcode = (OperationCode)op;

  if (recv_string(fd, req->key, sizeof(req->key), KEY_MAX) != 0)
    return -1;
  if (recv_string(fd, req->value1, sizeof(req->value1), VALUE1_MAX) != 0)
    return -1;
  if (recv_float_array(fd, req->V_value2, &req->N_value2) != 0)
    return -1;
  return recv_paquete(fd, &req->value3);
}

static int send_response_wire(int fd, const SockResponse *res) {
  if (send_i32(fd, res->status) != 0)
    return -1;
  if (send_string(fd, res->value1, VALUE1_MAX) != 0)
    return -1;
  if (send_float_array(fd, res->V_value2, res->N_value2) != 0)
    return -1;
  return send_paquete(fd, &res->value3);
}

void cleanup_and_exit(int sig) {
  (void)sig;
  if (server_fd != -1)
    close(server_fd);
  printf("\nNo more server.\n");
  printf("Peticiones procesadas: %d\n", counter);
  exit(0);
}

void *procesar_request(void *arg) {
  int client_fd = *(int *)arg;
  free(arg);

  SockRequest req;
  SockResponse res;
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  int status = -1;

  if (recv_request_wire(client_fd, &req) != 0) {
    close(client_fd);
    pthread_exit(NULL);
  }

  switch (req.opcode) {
  case OP_INIT:
    status = 0;
    break;

  case OP_DESTROY:
    status = destroy();
    break;

  case OP_SET:
    status = set_value(req.key, req.value1, req.N_value2, req.V_value2,
                       req.value3);
    break;

  case OP_GET:
    status = get_value(req.key, res.value1, &res.N_value2, res.V_value2,
                       &res.value3);
    break;

  case OP_MODIFY:
    status = modify_value(req.key, req.value1, req.N_value2, req.V_value2,
                          req.value3);
    break;

  case OP_DELETE:
    status = delete_key(req.key);
    break;

  case OP_EXIST:
    status = exist(req.key);
    break;

  default:
    status = -1;
    break;
  }

  res.status = status;

  if (send_response_wire(client_fd, &res) != 0)
    perror("send response");

  close(client_fd);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    return 1;
  }

  signal(SIGINT, cleanup_and_exit);
  signal(SIGTERM, cleanup_and_exit);

  struct addrinfo hints;
  struct addrinfo *result = NULL;
  struct addrinfo *rp;
  int gai_ret;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  gai_ret = getaddrinfo(NULL, argv[1], &hints, &result);
  if (gai_ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_ret));
    return 1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    int yes = 1;
    int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd == -1)
      continue;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      close(fd);
      continue;
    }

    if (bind(fd, rp->ai_addr, (socklen_t)rp->ai_addrlen) == 0) {
      server_fd = fd;
      break;
    }

    close(fd);
  }

  freeaddrinfo(result);

  if (server_fd == -1) {
    perror("bind");
    return 1;
  }

  if (listen(server_fd, 16) == -1) {
    perror("listen");
    close(server_fd);
    return 1;
  }

  printf("Socket server listening on port %s...\n", argv[1]);

  while (1) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
      if (errno == EINTR)
        continue;
      perror("accept");
      continue;
    }

    int *fd_ptr = malloc(sizeof(int));
    if (fd_ptr == NULL) {
      close(client_fd);
      continue;
    }
    *fd_ptr = client_fd;

    pthread_t tid;
    if (pthread_create(&tid, NULL, procesar_request, fd_ptr) != 0) {
      perror("pthread_create");
      close(client_fd);
      free(fd_ptr);
      continue;
    }

    pthread_detach(tid);
    counter++;
  }

  cleanup_and_exit(0);
  return 0;
}
