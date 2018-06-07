/*********************************************************************
** Program Filename: keygen.c
** Author: Kendra Ellis <ellisken@oregonstate.edu>
** Date: June 1, 2018
** Description: Generates a key of length n specified in argv[1] of all
        capital letters in the range A-Z with the addition of the
        space character.
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
int main(int argc, char *argv[]){
    //Seed PRNG
    srand(time(0));
    
    //Containers and variables
    int i;
    int num;
    int length = atoi(argv[1]);//Get specified length
    char *key = malloc(sizeof(char) * (length + 1));//Allocate string

    //Check for correct usage
    if(argc < 2){
        //Print usage message to stderr
        fprintf(stderr, "USAGE: %s key_length\n", argv[0]);
        exit(1);
    }

    //Generate a string of specified length of the appropriate chars
    //ASCII 32 and 65-90
    for(i=0; i < length; i++){
        //Generate random num in range 32 - 127 (ASCII 'space' to 'DEL')
        num = (rand() % 96) + 32;
        //If space, leave alone, else convert to char in range
        if(num == 32){
            key[i] = num;
        }
        else{
            num = (num % (26)) + 65;
            key[i] = num;
        }
    }

    //Add a terminating newline
    key[length] = '\n';

    //Print to stdout
    printf("%s", key);

    //Clean up
    free(key);

    return 0;
}
