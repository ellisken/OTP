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
#define MAX_SEND_LENGTH 1000 //Max # of chars sent at any given time

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
        printf("current char is %c\n", text[i]);
        //Search for a match among valid characters
        if(strchr(valid_chars, text[i]) == NULL){
            //If not found, send error
            error("Text entered is invalid\n");
        } 
    }
    //Else, text consists of only valid chars
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
	char buffer[SIZE], key[SIZE];
    FILE *file = NULL;

    
	if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args
    
    //Load plaintext and key into buffers
    load_from_file(buffer, file, argv[1]);
    load_from_file(key, file, argv[2]);
    //printf("plaintext: %s\n", buffer);
    //printf("key: %s\n", key);

    //Verify plaintext and key are valid contain only "legal"
    //characters, and that key >= plaintext in terms of size
    verify_text(buffer);
    verify_text(key);
    if(strlen(buffer) > strlen(key)){
        error("Error: key is too short\n");
    }


	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[2]); // Get the port number, convert to an integer from a string
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
    //Receive ok to send text
    //Send plaintext
    //Receive ok to send key
    //Send key
    //Wait to receive cipher
    //Print cipher to console

	/*// Get input message from user
	printf("CLIENT: Enter text to send to the server, and then hit enter: ");
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds

	// Send message to server
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	printf("CLIENT: I received this from the server: \"%s\"\n", buffer);*/

	close(socketFD); // Close the socket
	return 0;
}
