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

#define WORKER_N (5)
#define JOB_BUFFER_CAPACITY (5)

#define LINE_BUF_SIZE (2048)

typedef struct {

  // queue
  char jobs[JOB_BUFFER_CAPACITY][2048];

  size_t jobs_size;
  size_t jobs_rear;
  size_t jobs_front;

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

static const char* worker_get_job(worker_data_t* worker_data, buffer_t* buffer) {

  pthread_mutex_lock(&buffer->mutex);

  while (buffer->jobs_size == 0) {
    pthread_cond_wait(&buffer->can_consume, &buffer->mutex);
  }

  char* url = buffer->jobs[buffer->jobs_front];
  printf("[C][%ld] consuming %s ...\n", worker_data->worker_id, url);

  if (strcmp(url, "<EOF>")) {
    buffer->jobs_front = (buffer->jobs_front + 1) % JOB_BUFFER_CAPACITY;
    buffer->jobs_size = buffer->jobs_size - 1;
    pthread_cond_signal(&buffer->can_produce);
  }
  else {
    // Propagate EOF to the others workers
    pthread_cond_signal(&buffer->can_consume);
    url = NULL;
  }

  pthread_mutex_unlock(&buffer->mutex);

  return url;
}

static void* worker_run(void* arg) {

  worker_data_t* worker_data = (worker_data_t *)arg;
  buffer_t* buffer = worker_data->buffer;

  while (1) {
    
    const char* url = worker_get_job(worker_data, buffer);

    if (!url) {
      break;
    }

    /* const char* out_dir = "/dev/shm/screenshots/"; */
    /* Browser* browser = browser_create(out_dir); */
    /* browser_navigate_to(browser, url); */
  }

  return NULL;
}

static void setup_jobs_add(buffer_t* buffer, const char* url) {
  
  pthread_mutex_lock(&buffer->mutex);

  if (buffer->jobs_size == JOB_BUFFER_CAPACITY) {
    pthread_cond_wait(&buffer->can_produce, &buffer->mutex);
  }

  buffer->jobs_rear = (buffer->jobs_rear + 1) % JOB_BUFFER_CAPACITY;

  snprintf(buffer->jobs[buffer->jobs_rear],
	   sizeof(buffer->jobs[buffer->jobs_rear]),
	   "%s",
	   url);
  
  buffer->jobs_size = buffer->jobs_size + 1;

  printf("[P] adding %s ...\n", url);

  pthread_cond_signal(&buffer->can_consume);
  pthread_mutex_unlock(&buffer->mutex);
}

static void* setup_jobs(void* arg) {
  
  buffer_t* buffer = (buffer_t *)arg;

  FILE* fd = get_file("/dev/shm/urls.txt");
  char line[LINE_BUF_SIZE];

  while (fgets(line, sizeof(line), fd)) {
    line[strlen(line)-1] = '\0';

    if (!line || strlen(line) <= 0) {
      continue;
    }

    setup_jobs_add(buffer, line);
  }

  setup_jobs_add(buffer, "<EOF>");

  fclose(fd);

  return NULL;
}

static buffer_t* create_job_buffer() {

  buffer_t* buffer = malloc(sizeof(buffer_t));

  buffer->jobs_size = 0;
  buffer->jobs_front = 0;
  buffer->jobs_rear = JOB_BUFFER_CAPACITY - 1;

  pthread_cond_init(&buffer->can_produce, NULL);
  pthread_cond_init(&buffer->can_consume, NULL);
  pthread_mutex_init(&buffer->mutex, NULL);

  return buffer;
}

static void do_print(const char* target_url, const char* in_file, const char* out_dir) {

  if (1) {

    buffer_t* buffer = create_job_buffer();

    pthread_t producer;
    pthread_t workers[WORKER_N];
  
    if (pthread_create(&producer, NULL, setup_jobs, (void*)buffer)) {
      perror("Unable to create producer thread: ");
      exit(-1);
    }

    worker_data_t* worker_data[WORKER_N];
  
    for (int i = 0; i < WORKER_N; ++i) {
      worker_data[i] = malloc(sizeof(worker_data_t));
      worker_data[i]->buffer = buffer;
      worker_data[i]->worker_id = i;
      if (pthread_create(&workers[i], NULL, worker_run, (void *)worker_data[i])) {
	perror("Unable to create worker thread: ");
	exit(-1);
      }
    }

    pthread_join(producer, NULL);
    printf("pthread_join() producer - after\n");

    for (int i = 0; i < WORKER_N; ++i) {
      pthread_join(workers[i], NULL);
    }

    for (int i = 0; i < WORKER_N; ++i) {
      free(worker_data[i]);
      worker_data[i] = NULL;
    }

    free(buffer);
    buffer = NULL;

    // gtk_main_quit();

  }
//else {
    /* Browser* browser = browser_create(out_dir); */
    /* browser_navigate_to(browser, target_url); */
    //  }
}

int main(int argc, char** argv) {
  
  /* if (argc <= 1 || argc != 5) { */
  /*   exit(1); */
  /* } */

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

  /* if ((!in_file && !target_url) || !out_dir) { */
  /*   exit(1); */
  /* } */
  
  //XInitThreads();

  //gtk_init(&argc, &argv);

  do_print(target_url, in_file, out_dir);

  //gtk_main();
  
  return 0;
}
