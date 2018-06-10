/*********************************************************************
 * ** Program Filename: otp_dec.c
 * ** Author: Kendra Ellis <ellisken@oregonstate.edu>
 * ** Date: June 1, 2018
 * ** Description: Connects to otp_dec_d and asks it to perform a 
 *      one-time pad style decryption. Does not perform the decryption
 *      itself. Decryption is only successful if the same key used
 *      for encryption of the text is entered.
 * ** Input: otp_dec cipher key port
 * ** Output: Prints the plaintext received from otp_dec_d to stdout
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

#define SIZE 100000 //Buffer size
#define MAX_SEND_LENGTH 10//Max # of chars sent at any given time

/************************************************************************
 * ** Function: error()
 * ** Description: Prints the given message to stderr and exits with
 *      status 1.
 * ** Parameters: a constant char message
 * *********************************************************************/
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
   


/************************************************************************
 * ** Function: load_from_file()
 * ** Description: Loads the contents of the file matching "filename"
 *      into the given buffer, using the given file descriptor.
 *      Also strips the trailing newline off of the end of the file 
 *      contents.
 * ** Parameters: char buffer, file descriptor, filename
 * *********************************************************************/
void load_from_file(char *buffer, FILE *file, char *filename){
    int charsRead;
    //Zero out buffer
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
 * ** Function: verify_text()
 * ** Description: Checks that the given "text" contains ONLY upper case
 *      alpha characters OR the space character. Returns false otherwise.
 * ** Parameters: text
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
 *      a newline character is reached. Strips trailing newline char.
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
        strcat(buffer, readBuffer); //Add chunk to complete text
        if(chars_read == -1){
            error("ERROR reading from socket\n");
        }
        //Handle case where there is nothing to read
        if(chars_read == 0) break;
    }
    
    //Strip trailing newline
    buffer[strlen(buffer) - 1] = '\0';
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
void send_msg(int socketFD, char *buffer, char *msg){
    int chars_sent; //Used to track how many chars have been sent
    int message_length = strlen(msg); //Used to track how many chars (bytes) total to send
    memset(buffer, '\0', sizeof(buffer));
    strcpy(buffer, msg);
    char *current_location = msg; //Used to track where in the message we are, starts at beginning

    //Keep sending until entire message has been sent
    while(message_length > 0){
        //Send message in chunks of MAX_SEND_LENGTH
        chars_sent = send(socketFD, current_location, MAX_SEND_LENGTH, 0);
        //Check for errors
        if (chars_sent < 0) error("ERROR writing to socket\n");
        //Update remaining message length to send and current_location 
        message_length = message_length - chars_sent;
        current_location = current_location + chars_sent;
    }
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
	char buffer[SIZE], cipher[SIZE], key[SIZE];
    FILE *file = NULL;
    int yes = 1;//For port reuse

    //Verify correct usage
	if (argc < 4) { fprintf(stderr,"USAGE: %s cipher key port\n", argv[0]); exit(0); } // Check usage & args
    
    //Allow port reuse
    setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    
    //Load plaintext and key into buffers
    load_from_file(cipher, file, argv[1]);
    load_from_file(key, file, argv[2]);

    //Verify plaintext and key are valid contain only "legal"
    //characters, and that key >= plaintext in terms of size
    verify_text(cipher);
    verify_text(key);
    if(strlen(cipher) > strlen(key)){
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
	
	// Connect to server, send error if unsuccessful
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){// Connect socket to address
		fprintf(stderr, "ERROR: Could not connect to otp_dec_d on port %s\n", argv[3]);
        exit(2);
    }

    //Send authentication password "enc"
    send_msg(socketFD, buffer, "dec");

    //Receive authorized/unauthorized message and handle
    get_msg(buffer, socketFD);
    if(strcmp(buffer, "unauthorized") == 0){
		fprintf(stderr, "ERROR: Not authorized to connect to otp_enc_d\n");
        exit(2);
    }

    //Add ending newlines back to plaintext and key
    cipher[strlen(cipher)] = '\n';
    key[strlen(key)] = '\n';
    //Send cipher
    send_msg(socketFD, buffer, cipher); 
    //Send key
    send_msg(socketFD, buffer, key);
    //Wait to receive plaintext
    get_msg(buffer, socketFD);
    //Print plaintext to console
    printf("%s\n", buffer);
    
    //shutdown(socketFD, SHUT_WR);
	close(socketFD); // Close the socket
	return 0;
}
