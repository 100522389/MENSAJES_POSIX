#include "claves.h"
#include "common-mq.h"
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


mqd_t server_mq = -1;

void cleanup_and_exit(int sig) {
  if (server_mq != -1) {
    mq_close(server_mq);
    mq_unlink(SERVER_QUEUE);
  }
  printf("\nNo more server.\n");
  exit(0);
}

void *procesar_request(void *arg) {
  Request *req = (Request *)arg;
  Response res;
  memset(&res, 0, sizeof(Response));
  int status = -1;

  switch (req->opcode) {
  case OP_INIT:
    status = 0; // Trivial
    break;

  case OP_DESTROY:
    status = destroy();
    break;

  case OP_SET:
    status = set_value(req->key, req->value1, req->N_value2, req->V_value2,
                       req->value3);
    break;

  case OP_GET:
    status = get_value(req->key, res.value1, &res.N_value2, res.V_value2,
                       &res.value3);
    strncpy(req->value1, res.value1, 255);
    break;

  case OP_MODIFY:
    status = modify_value(req->key, req->value1, req->N_value2, req->V_value2,
                          req->value3);
    break;

  case OP_DELETE:
    status = delete_key(req->key);
    break;

  case OP_EXIST:
    status = exist(req->key);
    break;

  default:
    status = -1;
    break;
  }

  res.status = status;
  // Open client response queue to return the result
  mqd_t client_mq = mq_open(req->client_queue, O_WRONLY);
  if (client_mq != (mqd_t)-1) {
    if (mq_send(client_mq, (const char *)&res, sizeof(Response), 0) == -1) {
      perror("mq_send client queue");
    }
    mq_close(client_mq);
  } else {
    perror("mq_open client from server");
  }
  free(req);
  pthread_exit(NULL);
}

int main(void) {
  signal(SIGINT, cleanup_and_exit);
  signal(SIGTERM, cleanup_and_exit);

  // Clear up
  mq_unlink(SERVER_QUEUE);

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = sizeof(Request);
  attr.mq_curmsgs = 0;

  server_mq = mq_open(SERVER_QUEUE, O_CREAT | O_RDONLY, 0666, &attr);
  if (server_mq == (mqd_t)-1) {
    perror("mq_open server queue");
    exit(1);
  }

  printf("Server listening on queue %s...\n", SERVER_QUEUE);

  while (1) {
    Request *req = malloc(sizeof(Request));
    if (req == NULL) {
      perror("req");
      continue;
    }

    ssize_t bytes_read =
        mq_receive(server_mq, (char *)req, sizeof(Request), NULL);
    if (bytes_read == -1) {
      perror("mq_receive");
      free(req);
      continue;
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, procesar_request, req) != 0) {
      perror("pthread_create");
      free(req);
      continue;
    }

    // Detach
    pthread_detach(tid);
  }

  cleanup_and_exit(0);
  return 0;
}
