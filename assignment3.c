#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct Node* NodePtr;

#define BUFFER_SIZE 1024
#define MAX_THREADS 100

pthread_mutex_t thread_lock;

struct thread_state {
  int book_index;  // allocated by malloc and passed to thread
  pthread_t thread_id;
  pthread_t anaylis_thread_id; // only exists if search pattern != NULL
  int client_socket;  // socket to read from (connection to client)
};

int book_counter;
NodePtr book_starts[MAX_THREADS];

NodePtr head;
NodePtr end_list;

char* search_pattern = NULL;

int analysis_should_terminate = 0; // is set to 1 when the program terminates, tells the analysis thread to end 
pthread_mutex_t pattern_count_lock; 
int book_pattern_count[MAX_THREADS]; // how many times the search pattern occurs in each book


struct Node {
  int book_index;
  char data[BUFFER_SIZE];
  NodePtr book_next;
  NodePtr next;
};

NodePtr make_node(char* s, int book_index) {
  NodePtr node = (NodePtr)malloc(sizeof(struct Node));
  node->book_index = book_index;
  strcpy(node->data, s);
  node->book_next = NULL;
  node->next = NULL;

  return node;
}

void list_init(void) {
  head = make_node("NULL HEAD\n", -1);
  end_list = head;

  for (int i = 0; i < MAX_THREADS; i++) {
    book_starts[i] = NULL;
  }
}

// save the whole book to a file
void file_output(int book_index, NodePtr book_head) {
  char filename[12];
  if (book_index + 1 < 10) {
    sprintf(filename, "book_0%d.txt", book_index + 1);
  } else {
    sprintf(filename, "book_%d.txt", book_index + 1);
  }

  // output every line in book to book_xx.txt
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    printf("Error opening the file %s", filename);
    return;
  }

  // writing book to file
  NodePtr curr = book_head;

  while (curr != NULL) {
    fprintf(fp, "%s", curr->data);
    curr = curr->book_next;
  }

  fclose(fp);

  return;
}

void insert(char* s, int book_index, NodePtr book_head) {
  // add node to the end of linked list
  NodePtr new_node = make_node(s, book_index);

  pthread_mutex_lock(&thread_lock);

  end_list->next = new_node;
  end_list = new_node;

  // if this is a new book, add this head to the vector
  if (book_head == NULL) {
    book_starts[book_index] = new_node;
  } else {
    // add node to the end of book. first find book head
    NodePtr curr = book_starts[book_index];
    // now curr is the book head. traverse to the end of the book
    while (curr->book_next != NULL) {
      curr = curr->book_next;
    }
    // now curr is the last node of the book. insert here
    curr->book_next = new_node;
  }

  pthread_mutex_unlock(&thread_lock);

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


void* analysis_thread(void* args){

    while(analysis_should_terminate != 1){

        // analysis interval 
        sleep(3);

        // prints the next largest book while books_printed < book_counter
        int books_printed = 0;

        pthread_mutex_lock(&pattern_count_lock);
        pthread_mutex_lock(&thread_lock);

        // creating a local copy, enables us to mark printed counts as -1
        int book_pattern_count_mutable[MAX_THREADS];
        memcpy(book_pattern_count_mutable, book_pattern_count, sizeof(int) * MAX_THREADS);
        
        printf("-------------------\n");
        
        while(books_printed < book_counter){

            int max_index = -1;
            int max = -1;

            for(int i = 0; i < MAX_THREADS; i++){
                
                if(book_pattern_count_mutable[i] == -1){
                    continue;
                }

                if(book_pattern_count_mutable[i] >= max){
                    max_index = i;
                    max = book_pattern_count_mutable[i];
                }
            }

            books_printed++;

            if(max_index == -1){
                continue;
            }

            printf("%d occurances in %s\n", book_pattern_count_mutable[max_index], book_starts[max_index]->data);
            book_pattern_count_mutable[max_index] = -1;

        }
        printf("-------------------\n");

        pthread_mutex_unlock(&thread_lock);
        pthread_mutex_unlock(&pattern_count_lock);
        
    }
    return NULL;
}


void update_pattern_count(char* data, int book_index){

    // we are not looking for a pattern
    if(search_pattern == NULL){
        return;
    }


    pthread_mutex_lock(&pattern_count_lock);

    // increments how many times the string is found.

    char* to_search = data;
    while(to_search = strstr(to_search, search_pattern)){
        
        book_pattern_count[book_index]++;
        to_search++;
    }
    
    pthread_mutex_unlock(&pattern_count_lock);

}


void* connection_thread(void* args) {


  // extract thread data from void argument
  struct thread_state* thread_data;
  thread_data = (struct thread_state*)args;

  int socket = thread_data->client_socket;
  int book_index = thread_data->book_index;

  // buffer where client data will be read from
  char buffer[BUFFER_SIZE];
  int read_result;

    // init pattern count
    pthread_mutex_lock(&pattern_count_lock);
    book_pattern_count[book_index] = 0;
    pthread_mutex_unlock(&pattern_count_lock);

  do {
    read_result = check(recv(socket, buffer, BUFFER_SIZE - 1, 0),
                        "Failed to read from socket");
    insert(buffer, book_index, book_starts[book_index]);

    update_pattern_count(buffer, book_index);

    // print message for successfully added node
    printf("Node added to book %d\n", book_index);

    // clear the memory in the buffer
    memset(buffer, 0, BUFFER_SIZE);
  } while (read_result > 0);


  pthread_mutex_lock(&thread_lock);
  file_output(book_index, book_starts[book_index]);
  pthread_mutex_unlock(&thread_lock);

  close(socket);

  return NULL;
}

void resolve_connection(int socket) {
  struct thread_state* thread_args =
      (struct thread_state*)malloc(sizeof(struct thread_state));

  if (thread_args == NULL) {
    perror("Failed to allocate memory for thread_args");
    // Handle error and return if memory allocation fails
    return;
  }

  pthread_mutex_lock(&thread_lock);
  thread_args->book_index = book_counter++;
  pthread_mutex_unlock(&thread_lock);

  thread_args->client_socket = socket;

  // create our thread
  pthread_create(&thread_args->thread_id, NULL, connection_thread,
                 (void*)thread_args);

}

int main(int argc, char* argv[]) {
  book_counter = 0;
  list_init();

  check(pthread_mutex_init(&thread_lock, NULL),
        "Thread mutex failed to create\n");

  check(pthread_mutex_init(&pattern_count_lock, NULL),
        "Pattern mutex failed to create\n");

  int server_socket;
  int client_socket;

  struct sockaddr_in server_address;

  int server_port = atoi(argv[2]);
  if(argc > 4){
    search_pattern = argv[4];

    // -1 will represent no book in that location
    // when a connection is created, we mark that book as
    for(int i = 0; i < MAX_THREADS; i++){
        book_pattern_count[i] = -1;
    }
  }

  // create server
  // using TCP
  server_socket =
      check(socket(AF_INET, SOCK_STREAM, 0), "Failed creating server socket\n");

  // initialize server struct
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  // bind server, then listen...
  check(bind(server_socket, (struct sockaddr*)&server_address,
             sizeof(server_address)),
        "Failed binding server");
  check(listen(server_socket, 1), "Failed to listen to server");

  int address_size;

  int created_analysis_thread = 0;
  pthread_t analysis_thread_t;

  while (1) {
    address_size = sizeof(struct sockaddr_in);

    // accept() waiting for any incoming connections
    client_socket =
        check(accept(server_socket, (struct sockaddr*)&server_address,
                     (socklen_t*)&address_size),
              "Failed to accept connection\n");
    resolve_connection(client_socket);
    
    // create analysis thread if we have a search pattern and it does not already exist
    if(created_analysis_thread == 0 && search_pattern != NULL){
        created_analysis_thread = 1;
        pthread_create(&analysis_thread_t, NULL, analysis_thread, NULL);
    }
  }

  analysis_should_terminate = 1;
  pthread_join(analysis_thread_t, NULL);

  pthread_mutex_destroy(&thread_lock);
  pthread_mutex_destroy(&pattern_count_lock);

  return 0;
}