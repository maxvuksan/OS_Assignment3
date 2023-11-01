#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

/*

#pragma region book_list

struct Node* book_head;

struct Node {
  int book_index;
  char* data;
  struct Node* book_next = NULL;
  struct Node* next = NULL;
};

struct Node* make_node(char* s, int book_index) {
  struct Node* node = (struct Node*)malloc(sizeof(struct Node*));
  node->book_index = book_index;
  node->data = s;
  node->book_next = NULL;
  node->next = NULL;

  return node;
}

// save the whole book to a file
void file_output(int book_index) {

  // finds the head of a book given its connection index
  struct Node* curr = book_head;
  while (curr->next != NULL && curr->book_index != book_index) {
    curr = curr->next;
  }

  char* filename = "book_";
  if (book_index < 10) {
    filename += '0';
  }
  filename += itoa(book_index) + ".txt";

  // output every line in book to book_xx.txt
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    printf("Error opening the file %s", filename);
    return;
  }

 // writing book to file
  fprintf(fp, "%s\n", curr->data);
  curr = curr->book_next;

  while (curr != NULL) {
    char* line = curr->data;
    fprintf(fp, "%s\n", line);
    curr = curr->book_next;
  }

  fclose(fp);
}

void* insert(char* s, int book_index) {
  // add node to the end of regular linked list
  struct Node* new_node = make_node(s, book_index);
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


#pragma endregion
*/

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



/*
    assuming we have no more than 100 threads, we can define a static buffer to hold our thread objects, 
    when a thread is located it will search down the array and find the next free space
*/
#define MAX_THREADS 100

struct thread_state{
    
    int allocated = 0; // 1 if allocated
    int complete = 0; // 1 if complete
    int* book_index; // allocated by malloc and passed to thread
    pthread_t thread_id;
    int client_socket; // socket to read from (connection to client)
};

pthread_mutex_t thread_lock;
int book_counter = 0;
struct thread_state threads[MAX_THREADS];


void* connection_thread(void* void_book_index) {

    int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    int book_index = *((int*)(void_book_index));

    int read_result = 1;
   
    while(read_result > 0){

        read_result = check(read(threads[book_index].client_socket, buffer, BUFFER_SIZE), "Failed to read from socket");
    
        if(read_result == 0){
            break;
        }

        printf("%s", buffer);

        // clear the memory in the buffer
        memset(buffer, 0, BUFFER_SIZE);
    }
    

    // marking thread as complete
    pthread_mutex_lock(&thread_lock);
    
    threads[book_index].complete = 1;
    
    pthread_mutex_unlock(&thread_lock);
    
    return NULL;
}

void resolve_connection(int client_socket) {

    pthread_mutex_lock(&thread_lock);

    threads[book_counter].allocated = 1;
    threads[book_counter].complete = 0;
    threads[book_counter].book_index = (int*)malloc(sizeof(int));
    *threads[book_counter].book_index = book_counter;
    threads[book_counter].client_socket = client_socket;
    
    // create our thread
    pthread_create(&threads[book_counter].thread_id, NULL, connection_thread, (void*)threads[book_counter].book_index);

    book_counter++;
    
    pthread_mutex_unlock(&thread_lock);
}



int main(int argc, char* argv[]) {


    check(pthread_mutex_init(&thread_lock, NULL), "Thread mutex failed to create\n");

    int port = 1234;
    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    // create server
    // using TCP
    server_socket = check(socket(AF_INET, SOCK_STREAM, 0), "Failed creating server socket\n");

    // initialize server struct
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    // bind server, then listen...
    check(bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)), "Failed binding server");
    check(listen(server_socket, 1), "Failed to listen to server");

    int address_size;

    while (1) {
        address_size = sizeof(struct sockaddr_in);

        // accept() waiting for any incoming connections
        client_socket = check(accept(server_socket, (struct sockaddr*)&server_address, (socklen_t*)&address_size), "Failed to accept connection\n");

        resolve_connection(client_socket);


        // iterating over each thread determining if we any are ready to join
        for(int i = 0; i < MAX_THREADS; i++){
            
            // thread at index i exists and has finished running; join
            if(threads[i].allocated == 1 && threads[i].complete == 1){
                
                printf("thread ended\n");
                threads[i].allocated = 0;
                threads[i].complete = 0;
                free(threads[i].book_index);
                pthread_join(threads[i].thread_id, NULL);
            }
        }
    }

    pthread_mutex_destroy(&thread_lock); 



  return 0;
}
