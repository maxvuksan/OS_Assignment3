#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


// call when a problem occurs
void error(char* error_message){
    perror(error_message);
    exit(1); 
}

// a wrapper for all system calls which can fail,
// catches if the returned value is < 0
int check(int return_value, char* error_message){

    if(return_value < 0){
        error(error_message);
    }
    return return_value;
}

void resolve_connection(int client_socket){
    /*
        pthread_t new_thread;

        // create a new thread for the connect
        pthread_create(&new_thread, NULL, &connection_thread, NULL);
    */
}

void* connection_thread(){

}

int main(int argc, char *argv[]){

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
    check(bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)), "Failed binding server");
    check(listen(server_socket, 1), "Failed to listen to server");

    int address_size;

    while(1) {
        
        printf("Waiting for connections...\n");

        address_size = sizeof(struct sockaddr_in);
        client_socket = check(
            accept(server_socket, (struct sockaddr *) &client_address, (socklen_t*)&address_size),
            "Failed to accept connection\n");
        
        // connection success
        printf("Successfully Connected!\n");
        resolve_connection(client_socket);
    }

    return 0;
}
