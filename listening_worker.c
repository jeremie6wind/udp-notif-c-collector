#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include "listening_worker.h"
#include "parsing_worker.h"
#include "hexdump.h"
#include "unyte_utils.h"
#include "queue.h"

#define RCVSIZE 65565
#define RELEASE 1 /*Instant release of the socket on conn close*/
#define PARSER_NUMBER 10
#define QUEUE_SIZE 50

struct parse_worker
{
  queue_t *queue;
  pthread_t *worker;
};

/**
 * Udp listener worker on PORT port.
 */
int app(int port)
{
  /* debug */
  /* Sample of unyte_header */
  char test[] = {0x02, 0x0c, 0x00, 0x1b, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x7b, 0x22, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x22, 0x3a, 0x22, 0x54, 0x6f, 0x6d, 0x22, 0x7d};

  int release = RELEASE;
  struct sockaddr_in adresse;
  struct sockaddr_in from = {0};
  unsigned int fromsize = sizeof from;

  int infinity = 1;

  /* Create parsing workers */
  struct parse_worker *parsers = malloc(sizeof(struct parse_worker) * PARSER_NUMBER);
  if (parsers == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  /* Debug */
  char **j=malloc(sizeof(void *) * PARSER_NUMBER);
  for (int i = 0; i < PARSER_NUMBER; i++)
  {
    (parsers + i)->queue = (queue_t *)malloc(sizeof(queue_t));
    if ((parsers + i)->queue == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    (parsers + i)->queue->head = 0;
    (parsers + i)->queue->tail = 0;
    (parsers + i)->queue->size = QUEUE_SIZE;
    (parsers + i)->queue->data = malloc(sizeof(void *) * QUEUE_SIZE);
    sem_init(&(parsers + i)->queue->empty, 0, QUEUE_SIZE);
    sem_init(&(parsers + i)->queue->full, 0, 0);
    pthread_mutex_init(&(parsers + i)->queue->lock, NULL);

    if ((parsers + i)->queue->data == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    (parsers + i)->worker = (pthread_t *)malloc(sizeof(pthread_t));
    if ((parsers + i)->worker == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    pthread_create((parsers+i)->worker, NULL, t_parser, (void *) (parsers+i)->queue);

    *(j+i) = test;
    queue_write((parsers + i)->queue, *(j+i));
  }

  /*create socket on UDP protocol*/
  int server_desc = socket(AF_INET, SOCK_DGRAM, 0);

  /*handle error*/
  if (server_desc < 0)
  {
    perror("Cannot create socket\n");
    return -1;
  }

  setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &release, sizeof(int));

  adresse.sin_family = AF_INET;
  adresse.sin_port = htons((uint16_t)port);
  /* adresse.sin_addr.s_addr = inet_addr("192.0.2.2"); */
  adresse.sin_addr.s_addr = htonl(INADDR_ANY);

  /*initialize socket*/
  if (bind(server_desc, (struct sockaddr *)&adresse, sizeof(adresse)) == -1)
  {
    perror("Bind failed\n");
    close(server_desc);
    return -1;
  }

  /* Uncomment if no listening is wanted */
  infinity = 0;

  while (infinity)
  {
    int n;

    char *buffer = (char *)malloc(sizeof(char) * RCVSIZE);
    if (buffer == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    if ((n = recvfrom(server_desc, buffer, RCVSIZE - 1, 0, (struct sockaddr *)&from, &fromsize)) < 0)
    {
      perror("recvfrom failed\n");
      close(server_desc);
      return -1;
    }

    /* hexdump(buffer, n);
    fflush(stdout); */

    /* Parse and push to the unyte_segment queue */
    /* struct unyte_segment *segment = parse((char *)buffer);
    printHeader(&(segment->header), stdout); */

    struct unyte_minimal *seg = minimal_parse(buffer);

    /* printf("generator id : %d\n", seg->generator_id);
    printf("message id: %d\n", seg->generator_id); */

    /* Dispatching by modulo on threads */
    queue_write((parsers + (seg->generator_id % PARSER_NUMBER))->queue, seg);

    /* Comment if infinity is required */
    /* infinity = 0; */
  }
  /* Exit threads */
  char *end = "exit";
  for (int i = 0; i < PARSER_NUMBER; i++)
  {
    queue_write((parsers + i)->queue, end);
    pthread_join(*(parsers + i)->worker, NULL);
  }
  

  close(server_desc);
  return 0;
}

/**
 * Threadified app functin listening on *P port.
 */
void *t_app(void *p)
{
  app((int)*((int *)p));
  pthread_exit(NULL);
}
