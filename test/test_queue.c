#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../src/queue.h"

#define SIZE 10

void *t_read(void *queue)
{
  queue_t *output_queue = (queue_t *)queue;
  int read = 10;
  int read_done = 0;
  while (read--)
  {
    void *res = unyte_queue_read(output_queue);
    if (res == NULL)
    {
      printf("RES is NULL\n");
    }
    printf("Read %d\n", read_done++);
    // free(res);
  }
  return NULL;
}
void test_destructive()
{
  queue_t *queue = unyte_queue_init(SIZE);
  int counter = 0;
  char *buff;

  int msg_send = SIZE + 2;
  while (msg_send-- != 0)
  {
    buff = (char *)malloc(sizeof(int));
    sprintf(buff, "%d", counter);
    int ret = unyte_queue_destructive_write(queue, buff);
    printf("RET: %d|%d|%s\n", ret, msg_send, buff);
    counter++;
  }
  while (is_queue_empty(queue) != 0)
  {
    char *read_buff = (char *) unyte_queue_read(queue);
    printf("Read:%s\n", read_buff);
    free(read_buff);
  }
  free(queue->data);
  free(queue);
}

int main()
{
  // queue_t *output_queue = unyte_queue_init(SIZE);

  // // pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
  // // pthread_create(thread, NULL, t_read, (void *)output_queue);

  // char *buff = "Hello world!";
  // int msg_send = SIZE + 2;
  // while(msg_send-- != 0) {
  //   int ret = unyte_queue_write(output_queue, buff);
  //   printf("RET: %d|%d\n", ret, msg_send);
  // }

  // // pthread_join(*thread, NULL);
  // void *res = unyte_queue_read(output_queue);
  // if (res == NULL)
  // {
  //   printf("buffer is Null\n");
  // }
  test_destructive();
  return 0;
}
