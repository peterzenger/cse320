#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <getopt.h>

#include "pbx.h"
#include "server.h"
#include "debug.h"

#include "csapp.h"

/*
 * Thread function for the thread that handles a particular client.
 *
 * @param  Pointer to a variable that holds the file descriptor for
 * the client connection.  This variable must be freed once the file
 * descriptor has been retrieved.
 * @return  NULL
 *
 * This function executes a "service loop" that receives messages froms
 * the client and dispatches to appropriate functions to carry out
 * the client's requests.  The service loop ends when the network connection
 * shuts down and EOF is seen.  This could occur either as a result of the
 * client explicitly closing the connection, a timeout in the network causing
 * the connection to be closed, or the main thread of the server shutting
 * down the connection as part of graceful termination.
 */
int check_substring(char string[], char substring[], int start, int end){
    int check = 1;
    for (int i = start; i < end; i++){
        // debug("string: %c", string[i]);
        // debug("substring: %c", substring[i]);
        // debug("I: %d", i);
        if (string[i] != substring[i-start]){
            check = 0;
        }
    }
    return check;
}

int is_string_number(char string[]){
    int check = 1;
    for (int i = 0; i < strlen(string)-1; i++){
        if (!isdigit(string[i])){
            check = 0;
        }
    }
    return check;
}

void *pbx_client_service(void *arg){

    int client_fd, connection_fd;
    TU *client;
    connection_fd = *(int*)arg;
    free(arg);

    // debug("connection fd: %d", connection_fd);

    // detach so it doesn't have to be explicity reaped
    pthread_detach(pthread_self());

    // register the client file descriptor with pbx module
    if ((client = pbx_register(pbx, connection_fd)) == NULL){
        debug("Register failed");
        exit(EXIT_FAILURE);
    }

    if ((client_fd = tu_fileno(client)) == -1){
        debug("No file descriptor");
        exit(EXIT_FAILURE);
    }

    debug("client fd: %d", client_fd);

    // Make a buffer for a string

    char *message_buf;
    int c;
    int count;

    // char chat_buf[MAXLINE];
    FILE *client_pointer = fdopen(client_fd, "r");

    while (((c = fgetc(client_pointer)) != EOF)){
        count = 0;
        message_buf = malloc(sizeof(char) * (count+1));
        *(message_buf+count) = c;

        while(((c = fgetc(client_pointer)) != '\r')){
            count++;
            message_buf = realloc(message_buf, sizeof(char) * (count+1));
            *(message_buf+count) = c;
        }

        if (fgetc(client_pointer) != EOF){
            count++;
            message_buf = realloc(message_buf, sizeof(char) * count);
            *(message_buf+count) = '\0';
        } else {
            debug("error");
            return NULL;
        }

        // debug("%s",message_buf);

        if (check_substring(message_buf, "dial", 0, 4)){

            if (is_string_number(message_buf+5)){
                // debug("number %d", atoi(message_buf+5));
                tu_dial(client, atoi(message_buf+5));
            }
        } else if (check_substring(message_buf, "chat", 0, 4)){

            tu_chat(client, message_buf+5);

        } else {

            if (!strcmp(message_buf, "pickup")){
                tu_pickup(client);
            } else if (!strcmp(message_buf, "hangup")){
                tu_hangup(client);
            }
        }

        free(message_buf);
    }
    // if (message_buf != NULL){
    //     free(message_buf);
    //     // debug("free");
    // }
    // close(client_pointer);
    pbx_unregister(pbx, client);

    return NULL;
}