#include "claves.h"
#include "common-mq.h"
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// * Para declarar un puntero y desreferenciarlo, & para obtener la dir. de memoria

// Variable to store queue attributes
static struct mq_attr attr = {.mq_flags = 0,
                              .mq_maxmsg = 10,
                              .mq_msgsize = sizeof(Response),
                              .mq_curmsgs = 0};

// Helper function to send a request and wait for a response
static int send_request_and_wait(Request *req, Response *res) {
  mqd_t server_mq, client_mq;

  // Construct unique client queue name
  pid_t pid = getpid();
  pthread_t tid = pthread_self();
  snprintf(req->client_queue, MAX_QUEUE_NAME, "%s%d_%lu", CLIENT_QUEUE_PREFIX,
           pid, (unsigned long)tid);

  // Create and open client queue
  client_mq = mq_open(req->client_queue, O_CREAT | O_RDONLY, 0666, &attr);
  if (client_mq == (mqd_t)-1) {
    perror("mq_open client queue");
    return -2;
  }

  // Open server queue
  server_mq = mq_open(SERVER_QUEUE, O_WRONLY);
  if (server_mq == (mqd_t)-1) {
    perror("mq_open server queue");
    mq_close(client_mq);
    mq_unlink(req->client_queue);
    return -2;
  }

  // Send request to server
  if (mq_send(server_mq, (const char *)req, sizeof(Request), 0) == -1) {
    perror("mq_send");
    mq_close(server_mq);
    mq_close(client_mq);
    mq_unlink(req->client_queue);
    return -2;
  }

  // Wait for response
  ssize_t bytes_read =
      mq_receive(client_mq, (char *)res, sizeof(Response), NULL);
  if (bytes_read == -1) {
    perror("mq_receive");
    mq_close(server_mq);
    mq_close(client_mq);
    mq_unlink(req->client_queue);
    return -2;
  }

  // Cleanup
  mq_close(server_mq);
  mq_close(client_mq);
  mq_unlink(req->client_queue);

  return res->status;
}

int destroy(void) {
  Request req;
  Response res;

  memset(&req, 0, sizeof(Request));
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

  Request req;
  Response res;

  memset(&req, 0, sizeof(Request));
  req.opcode = OP_SET;
  strncpy(req.key, key, 255);
  strncpy(req.value1, value1, 255);
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

  Request req;
  Response res;

  memset(&req, 0, sizeof(Request));
  req.opcode = OP_GET;
  strncpy(req.key, key, 255);

  int status = send_request_and_wait(&req, &res);

  if (status == 0) {
    strncpy(value1, res.value1, 255);
    value1[255] = '\0';
    *N_value2 = res.N_value2;
    memcpy(V_value2, res.V_value2, res.N_value2 * sizeof(float));
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

  Request req;
  Response res;

  memset(&req, 0, sizeof(Request));
  req.opcode = OP_MODIFY;
  strncpy(req.key, key, 255);
  strncpy(req.value1, value1, 255);
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

  Request req;
  Response res;

  memset(&req, 0, sizeof(Request));
  req.opcode = OP_DELETE;
  strncpy(req.key, key, 255);

  return send_request_and_wait(&req, &res);
}

int exist(char *key) {
  if (key == NULL)
    return -1;
  if (strlen(key) > 255)
    return -1;

  Request req;
  Response res;

  memset(&req, 0, sizeof(Request));
  req.opcode = OP_EXIST;
  strncpy(req.key, key, 255);

  return send_request_and_wait(&req, &res);
}
