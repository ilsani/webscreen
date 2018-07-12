#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <webkit2/webkit2.h>

#include "browser.h"

#define WORKERS_N (5)
#define JOB_BUFFER_SIZE (5)

#define LINE_BUF_SIZE (2048)

pthread_mutex_t can_close_mutex;

typedef struct {

  // buffer
  char jobs[JOB_BUFFER_SIZE][2048];

  // number of items in the buffer
  size_t len;

  // add/remove data from buffer
  pthread_mutex_t mutex;

  // signaled when items are removed
  pthread_cond_t can_produce;

  // signaled when items are added;
  pthread_cond_t can_consume;
  
} buffer_t;

typedef struct {

  buffer_t* buffer;
  size_t worker_id;

} worker_data_t;

static void usage(const char* name) {
  
  printf("%s -i <input_file> -o <output_dir>\n", name);
}

static FILE* get_file(const char* filename) {
  
  FILE* fd;
  
  if (!(fd = fopen(filename, "r"))) {
    fprintf(stderr, "Cannot open %s: ", filename);
    perror(NULL);
    exit(1);
  }

  return fd;
}

static int worker_can_close(size_t worker_id, pthread_mutex_t* mutex) {

  switch (pthread_mutex_trylock(mutex)) {
    // if we got the lock, unlock and return true
  case 0:
    pthread_mutex_unlock(mutex);
    return 1;
    // return false if mutex was locked
  case EBUSY:
    return 0;
  }
  
  return 0;
}

static void* worker_run(void* arg) {

  worker_data_t* worker_data = (worker_data_t *)arg;
  buffer_t* buffer = worker_data->buffer;

  while (1) {

    pthread_mutex_lock(&buffer->mutex);

    while (buffer->len == 0 && !worker_can_close(worker_data->worker_id, &can_close_mutex)) {
      pthread_cond_wait(&buffer->can_consume, &buffer->mutex);
      sleep(1);
    }
    
    if (buffer->len > 0) {
      buffer->len = buffer->len - 1;
      const char* url = buffer->jobs[buffer->len];

      pthread_cond_signal(&buffer->can_produce);
      pthread_mutex_unlock(&buffer->mutex);
      
      if (!url || strlen(url) <= 0) {
	continue;
      }

      printf("[C][%ld] %s\n", worker_data->worker_id, url);

      const char* out_dir = "/dev/shm/screenshots/";
      Browser* browser = browser_create(out_dir);
      browser_navigate_to(browser, url);
    }
    else {
      pthread_mutex_unlock(&buffer->mutex);
      break;
    }

  }

  return NULL;
}

static void* worker_setup_jobs(void* arg) {
  
  buffer_t* buffer = (buffer_t *)arg;

  FILE* fd = get_file("/dev/shm/urls.txt");
  char line[LINE_BUF_SIZE];

  while (fgets(line, sizeof(line), fd)) {
    line[strlen(line)-1] = '\0';

    if (!line || strlen(line) <= 0) {
      continue;
    }

    pthread_mutex_lock(&buffer->mutex);

    if (buffer->len == JOB_BUFFER_SIZE) {
      pthread_cond_wait(&buffer->can_produce, &buffer->mutex);
    }

    snprintf(buffer->jobs[buffer->len], sizeof(buffer->jobs[buffer->len]), "%s", line);
    buffer->len = buffer->len + 1;

    pthread_cond_signal(&buffer->can_consume);
    pthread_mutex_unlock(&buffer->mutex);
  }

  fclose(fd);

  return NULL;
}

static void do_print(const char* target_url, const char* in_file, const char* out_dir) {

  if (!target_url) {

    buffer_t buffer = {
      .len = 0,
      .can_produce = PTHREAD_COND_INITIALIZER,
      .can_consume = PTHREAD_COND_INITIALIZER
    };
  
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_mutex_lock(&can_close_mutex);

    pthread_t producer;
    pthread_t workers[WORKERS_N];

    pthread_create(&producer, NULL, worker_setup_jobs, (void*)&buffer);
  
    for (int i = 0; i < WORKERS_N; ++i) {

      worker_data_t* worker_data = malloc(sizeof(worker_data_t));
      worker_data->buffer = &buffer;
      worker_data->worker_id = i;
    
      pthread_create(&workers[i], NULL, worker_run, (void *)worker_data);
    }

    pthread_join(producer, NULL);

    pthread_cond_broadcast(&buffer.can_consume);
    pthread_mutex_unlock(&can_close_mutex);

    for (int i = 0; i < WORKERS_N; ++i) {
      pthread_join(workers[i], NULL);
    }

    gtk_main_quit();

  }
  else {
    /* Browser* browser = browser_create(out_dir); */
    /* browser_navigate_to(browser, target_url); */
  }
}

int main(int argc, char** argv) {
  
  if (argc <= 1 || argc != 5) {
    exit(1);
  }

  int opt;
  char* in_file = NULL;
  char* out_dir = NULL;
  char* target_url = NULL;
  
  while ((opt = getopt(argc, argv, "u:i:o:")) != -1) {

    switch (opt) {

    case 'u':
      target_url = optarg;

    case 'i':
      in_file = optarg;
      break;

    case 'o':
      out_dir = optarg;
      break;

    }
  }

  if ((!in_file && !target_url) || !out_dir) {
    exit(1);
  }

  XInitThreads();

  gtk_init(&argc, &argv);

  do_print(target_url, in_file, out_dir);

  gtk_main();
  
  return 0;
}
