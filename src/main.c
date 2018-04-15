#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <webkit2/webkit2.h>

#include "browser.h"

#define LINE_BUF_SIZE (2048)

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

static void do_print(const char* target_url, const char* in_file, const char* out_dir) {

  Browser* browser = browser_create(out_dir);

  if (target_url) {
    browser_navigate_to(browser, target_url);
  }
  else {

    // TODO

    /* FILE* fd = get_file(in_file); */

    /* // TODO: read char by char */
    /* char line[LINE_BUF_SIZE]; */
  
    /* while (fgets(line, sizeof(line), fd)) { */
      
    /*   line[strlen(line)-1] = '\0'; */
    
    /*   if (!line || strlen(line) < 1) */
    /* 	continue; */

    /*   browser_navigate_to(browser, line); */

    /* } */

    /* fclose(fd); */
    
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

  gtk_init(&argc, &argv);

  do_print(target_url, in_file, out_dir);

  gtk_main();
  
  return 0;
}
