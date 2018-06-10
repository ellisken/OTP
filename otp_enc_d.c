/*********************************************************************
 * ** Program Filename: otp_enc_d.c
 * ** Author: Kendra Ellis <ellisken@oregonstate.edu>
 * ** Date: June 1, 2018
 * ** Description: Allows connections from otp_enc.c and encrypts
 *      a plaintext file using the given key. Sends the cipher back
 *      to otp_enc.c
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
#include <stdbool.h>

#define SIZE 100000 //Buffer size
#define MAX_SEND_LENGTH 10 //Max # of chars sent at any given time
#define MAX_CONNECTIONS 5 //Max number of socket connections



/************************************************************************
 * ** Function: error()
 * ** Description: Sends "msg" to stderr and exits with value 1.
 * ** Parameters: msg
 * *********************************************************************/
void error(const char *msg) { perror(msg); exit(1); }



/************************************************************************
 * ** Function: get_msg()
 * ** Description: Reads in a message sent from the client until
 *      a newline character is reached. Strips trailing newline.
 * ** Parameters: char buffer for storing complete message,
 *      file descriptor for connection socket.
 * *********************************************************************/
void get_msg(char *buffer, int socketFD){
    int chars_read; //For checking correct read
    char readBuffer[MAX_SEND_LENGTH];
    
    //Clear buffer
    memset(buffer, '\0', sizeof(buffer));

    //As long as we have not reached a newline
    while(strstr(buffer, "\n") == NULL){
        //Clear readBuffer
        memset(readBuffer, '\0', sizeof(readBuffer));
        chars_read = recv(socketFD, readBuffer, sizeof(readBuffer)-1, 0); //Get a chunk of text
        /*int i;
        printf("received chars: %d\n", chars_read);
        for(i=0; i < chars_read; i++){
            printf("readBuffer: %c\n", readBuffer[i]);
        }*/
        strcat(buffer, readBuffer); //Add chunk to complete text
        if(chars_read == -1){
            error("ERROR reading from socket");
        }
        //Handle case where there is nothing to read
        if(chars_read == 0) break;
    }

    //Strip trailing newline
    buffer[strlen(buffer) - 1] = '\0';

    return;
}



/************************************************************************
 * ** Function: authenticate_client()
 * ** Description: Gets the first message sent from the client. If  
 *      message received == "enc", return true, else return false.
 * ** Parameters: a char buffer for storing the message, the connection
 *      socket file descriptor
 * *********************************************************************/
bool authenticate_client(char *buffer, int socketFD){
    int chars_read; //For checking correct read

    //Clear buffer
    memset(buffer, '\0', sizeof(buffer));

    //Read from socket
    chars_read = recv(socketFD, buffer, SIZE-1, 0);

    //Check for success
    if(chars_read < 0) error ("ERROR reading from socket");

    //If msg is "enc", return true
    if(strcmp(buffer, "enc") == 0){
        return true;
    }

    else return false;
}



/************************************************************************
 * ** Function: send_msg()
 * ** Description: Sends a specified message or text to the client over
 *      the specified connection socket in chunks to ensure entire
 *      message is sent
 * ** Parameters: The connection socket file desriptor
 *      and a constant char message to send.
 * *********************************************************************/
void send_msg(int socketFD, char *buffer, char *msg){
    int chars_sent; //Used to track how many chars have been sent
    int message_length = strlen(msg); //Used to track how many chars (bytes) total to send 
    memset(buffer, '\0', sizeof(buffer));
    strcpy(buffer, msg);
    char *current_location = buffer; //Used to track where in the message we are, starts at beginning

    //Keep sending until entire message has been sent
    while(message_length > 0){
        //Send message in chunks of max_send_length
        chars_sent = send(socketFD, current_location, MAX_SEND_LENGTH, 0);
        //Check for errors
        if (chars_sent < 0) error("ERROR writing to socket");
        //Update remaining message length to send and current_location 
        message_length = message_length - chars_sent;
        current_location = current_location + chars_sent;
    }
    return;
}



/************************************************************************
 * ** Function: ctoi()
 * ** Description: Converts a given upper case alpha char into its 
 *      corresponding number in the alphabet (e.g., A = 0, Z = 25), 
 *      returns 26 for a space character.
 * ** Parameters: a char
 * *********************************************************************/
int ctoi(char c){
    int n;
    //If c is a space character, return 26
    if(c == ' ') return 26;

    //Subract the capital letter A to get the correct int
    else{
        n = c - 'A';
        return n;
    }
}



/************************************************************************
 * ** Function: itoc()
 * ** Description: Converts a given int back into the corresponding
 *      upper case alpha char. Returns a space char for the number 26.
 * ** Parameters: an int 
 * *********************************************************************/
char itoc(int n){
    //If n is 26, return a space
    if(n == 26) return ' ';
    //Else, find correct ascii for n by adding A
    else return (n + 'A');
}



/************************************************************************
 * ** Function: encrypt()
 * ** Description: Encrypts the plaintext message using the given key
 *      by assigning a number value to each plaintext char and key char,
 *      adding them together, and taking modulo 27.
 * ** Parameters: The plaintext array, the key array, the cipher array
 * *********************************************************************/
void encrypt(char *text, char *key, char *cipher){
    int i;
    int n;
    
    //Zero out the cipher
    memset(cipher, '\0', sizeof(cipher));

    //Encrypt
    for(i=0; i < strlen(text); i++){
        //Convert text and key chars to ints, sum
        n = (ctoi(text[i]) + ctoi(key[i])) % 27;
        //Convert result back into char and store in cipher
        cipher[i] = itoc(n);
    }

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
    int signal;
    
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
	char buffer[SIZE], key[SIZE], cipher[SIZE];
	struct sockaddr_in serverAddress, clientAddress;
    pid_t pid; //child process id
    int yes = 1; //For port reuse

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

    //Allow port reuse
    setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

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
		error("otp_enc_d ERROR on binding");

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
                //Authenticate client
                if(authenticate_client(buffer, establishedConnectionFD) == false){
                    //Send error message
                    send_msg(establishedConnectionFD, buffer, "unauthorized\n");
                    break;
                }
                else{
                    //Send acknowledgment
                    send_msg(establishedConnectionFD, buffer, "authorized\n");
                    //Get Text
                    get_msg(buffer, establishedConnectionFD);
                    //Get Key
                    get_msg(key, establishedConnectionFD);
                    //Encrypt
                    encrypt(buffer, key, cipher);
                    //Add newline to end of cipher
                    cipher[strlen(cipher)] = '\n';
                    //Send cipher
                    send_msg(establishedConnectionFD, buffer, cipher);
                    sleep(1);//Sorry this is a kludgy way to keep the connection open long enough for transmission
                    //Zero out the buffer
                    memset(buffer, '\0', sizeof(buffer));
                    break;
                }
            case -1:
                error("otp_enc_d ERROR forking process");
                break; 
        }
        close(establishedConnectionFD);//Close connection socket
        check_background();//Clean up child processes
    }
    close(listenSocketFD);//Close listening socket
	return 0; 
}
