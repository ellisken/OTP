/*********************************************************************
 * ** Program Filename: otp_enc_d.c
 * ** Author: Kendra Ellis <ellisken@oregonstate.edu>
 * ** Date: June 1, 2018
 * ** Description:
 * ** Input:
 * ** Output:
 * *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define SIZE 1000 //Buffer size
#define MAX_CONNECTIONS 5 //Max number of socket connections

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

void encrypt(){

    return;
}



/************************************************************************
 * ** Function: check_background()
 * ** Description: Checks for the background child processes that have
 *      finished and cleans up zombies
 * ** Parameters: none
 * *********************************************************************/
void check_background(){
    pid_t cpid; //For storing the terminated process's id
    
    //Wait for any child process, return immediately if none have exited
    cpid = waitpid(-1, &signal, WNOHANG);
    while(cpid > 0){
        cpid = waitpid(-1, &signal, WNOHANG);
    }
    return;
}



/***********************************************************************
 *                                 MAIN
 * ********************************************************************/
int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[SIZE];
	struct sockaddr_in serverAddress, clientAddress;
    pid_t pid; //child process id 

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("otp_end_d ERROR on binding");

	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

    while(1){ //Start listening continuously for connections

        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
        if (establishedConnectionFD < 0) error("otp_enc_d ERROR on accept");

        // Fork another process to handle the encryption
        pid = fork();
        switch(pid){
            case 0: //Successful fork, handles the connection with encryption
                memset(buffer, '\0', SIZE); //Zero out buffer
                printf("Connection successful!\n");
                break;
            case -1:
                error(stderr,"otp_enc_d ERROR forking process");
                break; 
        }
        close(establishedConnectionFD);//Close connection socket
        check_background();//Clean up child processes

        /*// Get the message from the client and display it
        memset(buffer, '\0', SIZE);
        charsRead = recv(establishedConnectionFD, buffer, SIZE - 1, 0); // Read the client's message from the socket
        if (charsRead < 0) error("ERROR reading from socket");
        printf("SERVER: I received this from the client: \"%s\"\n", buffer);

        // Send a Success message back to the client
        charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
        if (charsRead < 0) error("ERROR writing to socket");*/
    }
    close(listenSocketFD);//Close listening socket
	return 0; 
}
