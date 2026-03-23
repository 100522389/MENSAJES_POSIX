#include "claves.h"
#include "common-mq.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ENV_IP "IP_TUPLAS"
#define ENV_PORT "PORT_TUPLAS"
#define KEY_MAX 255
#define VALUE1_MAX 255
#define MAX_VECTOR 32

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

static int send_request_wire(int fd, const SockRequest *req) {
  if (send_u32(fd, (uint32_t)req->opcode) != 0)
    return -1;
  if (send_string(fd, req->key, KEY_MAX) != 0)
    return -1;
  if (send_string(fd, req->value1, VALUE1_MAX) != 0)
    return -1;
  if (send_float_array(fd, req->V_value2, req->N_value2) != 0)
    return -1;
  return send_paquete(fd, &req->value3);
}

static int recv_response_wire(int fd, SockResponse *res) {
  if (recv_i32(fd, &res->status) != 0)
    return -1;
  if (recv_string(fd, res->value1, sizeof(res->value1), VALUE1_MAX) != 0)
    return -1;
  if (recv_float_array(fd, res->V_value2, &res->N_value2) != 0)
    return -1;
  return recv_paquete(fd, &res->value3);
}

static int connect_to_server(void) {
  const char *host = getenv(ENV_IP);
  const char *port = getenv(ENV_PORT);
  if (host == NULL || host[0] == '\0' || port == NULL || port[0] == '\0') {
    fprintf(stderr, "Missing env vars %s or %s\n", ENV_IP, ENV_PORT);
    return -1;
  }

  struct addrinfo hints;
  struct addrinfo *result = NULL;
  struct addrinfo *rp;
  int fd = -1;
  int gai_ret;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  gai_ret = getaddrinfo(host, port, &hints, &result);
  if (gai_ret != 0) {
    fprintf(stderr, "getaddrinfo(%s:%s): %s\n", host, port,
            gai_strerror(gai_ret));
    return -1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd == -1)
      continue;
    if (connect(fd, rp->ai_addr, (socklen_t)rp->ai_addrlen) == 0)
      break;
    close(fd);
    fd = -1;
  }

  freeaddrinfo(result);
  return fd;
}

// Helper function to send a request and wait for a response
static int send_request_and_wait(const SockRequest *req, SockResponse *res) {
  int fd = connect_to_server();
  if (fd < 0)
    return -2;

  if (send_request_wire(fd, req) != 0) {
    perror("send request wire");
    close(fd);
    return -2;
  }

  if (recv_response_wire(fd, res) != 0) {
    perror("recv response wire");
    close(fd);
    return -2;
  }

  close(fd);

  return res->status;
}

int destroy(void) {
  SockRequest req;
  SockResponse res;

  memset(&req, 0, sizeof(req));
  req.opcode = OP_DESTROY;

  return send_request_and_wait(&req, &res);
}

int set_value(char *key, char *value1, int N_value2, float *V_value2,
              struct Paquete value3) {
  if (key == NULL || value1 == NULL || V_value2 == NULL)
    return -1;
  if (N_value2 < 1 || N_value2 > 32)
    return -1;
  if (strlen(value1) > 255 || strlen(key) > 255)
    return -1;

  SockRequest req;
  SockResponse res;

  memset(&req, 0, sizeof(req));
  req.opcode = OP_SET;
  strncpy(req.key, key, KEY_MAX);
  req.key[KEY_MAX] = '\0';
  strncpy(req.value1, value1, VALUE1_MAX);
  req.value1[VALUE1_MAX] = '\0';
  req.N_value2 = N_value2;
  memcpy(req.V_value2, V_value2, N_value2 * sizeof(float));
  req.value3 = value3;

  return send_request_and_wait(&req, &res);
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2,
              struct Paquete *value3) {
  if (key == NULL || value1 == NULL || N_value2 == NULL || V_value2 == NULL ||
      value3 == NULL)
    return -1;
  if (strlen(key) > 255)
    return -1;

  SockRequest req;
  SockResponse res;

  memset(&req, 0, sizeof(req));
  req.opcode = OP_GET;
  strncpy(req.key, key, KEY_MAX);
  req.key[KEY_MAX] = '\0';

  int status = send_request_and_wait(&req, &res);

  if (status == 0) {
    strncpy(value1, res.value1, VALUE1_MAX);
    value1[VALUE1_MAX] = '\0';
    *N_value2 = res.N_value2;
    memcpy(V_value2, res.V_value2, (size_t)res.N_value2 * sizeof(float));
    *value3 = res.value3;
  }

  return status;
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2,
                 struct Paquete value3) {
  if (key == NULL || value1 == NULL || V_value2 == NULL)
    return -1;
  if (N_value2 < 1 || N_value2 > 32)
    return -1;
  if (strlen(value1) > 255 || strlen(key) > 255)
    return -1;

  SockRequest req;
  SockResponse res;

  memset(&req, 0, sizeof(req));
  req.opcode = OP_MODIFY;
  strncpy(req.key, key, KEY_MAX);
  req.key[KEY_MAX] = '\0';
  strncpy(req.value1, value1, VALUE1_MAX);
  req.value1[VALUE1_MAX] = '\0';
  req.N_value2 = N_value2;
  memcpy(req.V_value2, V_value2, N_value2 * sizeof(float));
  req.value3 = value3;

  return send_request_and_wait(&req, &res);
}

int delete_key(char *key) {
  if (key == NULL)
    return -1;
  if (strlen(key) > 255)
    return -1;

  SockRequest req;
  SockResponse res;

  memset(&req, 0, sizeof(req));
  req.opcode = OP_DELETE;
  strncpy(req.key, key, KEY_MAX);
  req.key[KEY_MAX] = '\0';

  return send_request_and_wait(&req, &res);
}

int exist(char *key) {
  if (key == NULL)
    return -1;
  if (strlen(key) > 255)
    return -1;

  SockRequest req;
  SockResponse res;

  memset(&req, 0, sizeof(req));
  req.opcode = OP_EXIST;
  strncpy(req.key, key, KEY_MAX);
  req.key[KEY_MAX] = '\0';

  return send_request_and_wait(&req, &res);
}
