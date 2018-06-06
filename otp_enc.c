/*********************************************************************
 * ** Program Filename: otp_enc.c
 * ** Author: Kendra Ellis <ellisken@oregonstate.edu>
 * ** Date: June 1, 2018
 * ** Description: Connects to otp_enc_d and asks it to perform a 
 *      one-time pad style encryption. Does not perform the encryption
 *      itself. 
 * ** Input: otp_enc plaintext key port
 * ** Output: Prints the ciphertext from otp_enc_d to stdout
 * *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>

#define SIZE 10000 //Buffer size
#define MAX_SEND_LENGTH 100//Max # of chars sent at any given time

/************************************************************************
 * ** Function: check_background()
 * ** Description: Checks for the background child processes that have
 *      finished and cleans up zombies
 * ** Parameters: none
 * *********************************************************************/
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
   


/************************************************************************
 * ** Function: check_background()
 * ** Description: Checks for the background child processes that have
 *      finished and cleans up zombies
 * ** Parameters: none
 * *********************************************************************/
void load_from_file(char *buffer, FILE *file, char *filename){
    int charsRead;

    memset(buffer, '\0', sizeof(buffer));
    //Open file
    file = fopen(filename, "r");
    if(file == NULL){
        error("Can't open file.\n");
    }
    //Read from file
    while(!feof(file)){
        charsRead = fread(buffer, sizeof(char), SIZE, file);
    }
    //Strip trailing newline char
    buffer[strlen(buffer) - 1] = '\0';

    //Close file
    fclose(file);
    return;
}



/************************************************************************
 * ** Function: check_background()
 * ** Description: Checks for the background child processes that have
 *      finished and cleans up zombies
 * ** Parameters: none
 * *********************************************************************/
void verify_text(char *text){
    int i;
    int length = strlen(text);
    char valid_chars[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    //Verify that each char is valid
    for(i=0; i < length; i++){
        //Search for a match among valid characters
        if(strchr(valid_chars, text[i]) == NULL){
            //If not found, send error
            error("Text entered is invalid\n");
        } 
    }
    //Else, text consists of only valid chars
    return;
}



/************************************************************************
 * ** Function: get_msg()
 * ** Description: Reads in a message sent from the client until
 *      a newline character is reached.
 * ** Parameters: char buffer for storing complete message,
 *      file descriptor for connection socket.
 * *********************************************************************/
void get_msg(char *buffer, int socketFD){
    int chars_read; //For checking correct read
    char readBuffer[1];
    
    //Clear buffer
    memset(buffer, '\0', sizeof(buffer));

    //As long as we have not reached a newline
    while(strstr(buffer, "\n") == NULL){
        //Clear readBuffer
        memset(readBuffer, '\0', sizeof(readBuffer));
        chars_read = recv(socketFD, readBuffer, sizeof(readBuffer)-1, 0); //Get a chunk of text
        strcat(buffer, readBuffer); //Add chunk to complete text
        if(chars_read == -1){
            error("ERROR reading from socket");
        }
        //Handle case where there is nothing to read
        if(chars_read == 0) break;
    }
    return;
}



/************************************************************************
 * ** Function: send_msg()
 * ** Description: Sends a specified message or text to the client over
 *      the specified connection socket in chunks to ensure entire
 *      message is sent
 * ** Parameters: The connection socket file desriptor
 *      and a constant char message to send.
 * *********************************************************************/
void send_msg(int socketFD, char *msg){
    int chars_sent; //Used to track how many chars have been sent
    int message_length = strlen(msg); //Used to track how many chars (bytes) total to send 
    char *current_location = msg; //Used to track where in the message we are, starts at beginning

    //Keep sending until entire message has been sent
    while(message_length > 0){
        //Send message in chunks of MAX_SEND_LENGTH
        chars_sent = send(socketFD, current_location, MAX_SEND_LENGTH, 0);
        //Check for errors
        if (chars_sent < 0) error("ERROR writing to socket");
        //Update remaining message length to send and current_location 
        message_length = message_length - chars_sent;
        current_location = current_location + chars_sent;
    }
    //send(socketFD, "\n", 1, 0);
    return;
}



/***********************************************************************
 *                                 MAIN
 * ********************************************************************/
int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[SIZE], text[SIZE], key[SIZE];
    FILE *file = NULL;

    
	if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args
    
    //Load plaintext and key into buffers
    load_from_file(text, file, argv[1]);
    load_from_file(key, file, argv[2]);
    //printf("Text: %s\n", text);
    //printf("Key: %s\n", key);


    //Verify plaintext and key are valid contain only "legal"
    //characters, and that key >= plaintext in terms of size
    verify_text(text);
    verify_text(key);
    if(strlen(text) > strlen(key)){
        error("Error: key is too short\n");
    }


	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

    //Send authentication password "enc"
    send_msg(socketFD, "enc");

    //Receive authorized/unauthorized message and handle
    get_msg(buffer, socketFD);
    if(strcmp(buffer, "unauthorized") == 0){
        error("Error: unauthorized connection to server\n");    
    }

    //Add ending newlines back to plaintext and key
    text[strlen(text)] = '\n';
    key[strlen(key)] = '\n';
    //Send plaintext
    send_msg(socketFD, text); 
    //Receive ok to send key
    //get_msg(buffer, socketFD);
    //printf("Server sent: %s\n", buffer);
    printf("Now sending key...\n");
    //Send key
    //sleep(5);
    send_msg(socketFD, key);
    sleep(1);
    //Wait to receive cipher
    printf("Now accepting cipher...\n");
    get_msg(buffer, socketFD);
    //Print cipher to console
    printf("%s\n", buffer);
    
    shutdown(socketFD, SHUT_WR);
	close(socketFD); // Close the socket
	return 0;
}
