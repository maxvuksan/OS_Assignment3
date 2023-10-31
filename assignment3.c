#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

typedef struct sockaddr_in SA_IN;

#define TRUE 1
#define FALSE 0

#define SERVER_PORT 12345

pthread_t threads[100];

struct Node* lookup_head;
struct Node* end_list;

struct Node {
  char* data;
  struct Node* book_next;
  struct Node* next;
};

void listInit() {
  lookup_head = makeNode("NOT A NODE");
  end_list = makeNode("NOT A NODE");
}

struct Node* makeNode(char* s) {
  struct Node* node = (struct Node*)malloc(sizeof(struct Node*));
  node->data = s;
  node->book_next = NULL;
  node->next = NULL;

  return node;
}

// save the whole book to a file
void fileOutput(struct Node* head) {
  // work out which number book this is
  int i = 1;
  struct Node* curr = lookup_head->next;
  while (curr != head) {
    curr = curr->next;
    i++;
  }

  char* filename = "book_";
  if (i < 10) {
    filename += '0';
  }
  filename += itoa(i) + '.txt';

  // output every line in book to book_xx.txt
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    printf("Error opening the file %s", filename);
    return -1;
  }

  // curr = pointer to
  curr = head->book_next;

  while (curr != NULL) {
    char* line = curr->data;
    fprintf(fp, "%s\n", line);
    curr = curr->book_next;
  }

  fclose(fp);
}

void* insert(char* s, struct Node* book_head) {
  // add node to the end of regular linked list
  struct Node* new_node = makeNode(s);
  end_list->next = new_node;
  end_list = new_node;

  // add node to the end of book
  struct Node* curr = book_head;
  while (curr->book_next != NULL) {
    curr = curr->book_next;
  }
  // now curr is the last node of book before adding new node
  curr->book_next = new_node;

  return;
}

// call when a problem occurs
void error(char* error_message) {
  perror(error_message);
  exit(1);
}

// a wrapper for all system calls which can fail,
// catches if the returned value is < 0
int check(int return_value, char* error_message) {
  if (return_value < 0) {
    error(error_message);
  }
  return return_value;
}

void handle_connection(int client_socket) {}

void* connection_thread() {}

main(int argc, char* argv[]) {
  int server_socket, client_socket, addr_size;
  SA_IN server_addr, client_addr;

  check((server_socket = socket(AF_INET, SOCK_STREAM, 0)),)

  return 0;
}