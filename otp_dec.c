/*******************************************************************************
** Program Name: otp_dec.c
** Author:       Story Caplain
** Last Updated: 11/05/19
** Description:  This program connects to otp_dec_d 
**               And asks otp_dec_d to decrypt the ciphertext
**               By itself, otp_dec doesn’t do the decryption, otp_dec_d does.
**               otp_dec receives the deciphered text back from otp_dec_d  
**               And outputs the decciphered text to stdout.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h> 
#include <errno.h>
#define SIZE 100000

/* Error function used for reporting issues */
void error(const char *msg) { fprintf(stderr,"%s\n",msg); exit(1); }

/* counts the number of chars in a file who's name is passed in */
int num_of_chars(const char*);

/* connect to otp_dec_d and send message for decryption */
void connector(char*,int,int);

int main(int argc, char *argv[])
{
  /* variables to be used below */
  int socketFD, portNumber, charsWritten, charsRead, temp;
  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;
  char buffer[SIZE];
  char temp_buffer[SIZE];
  char err_msg[SIZE];
    
  if(argc != 4) { 
      error("Incorrect number of args"); 
  } /* Check usage & args */

  /* check that key is at least as long as message to decode */
  int dec_length = num_of_chars(argv[1]);
  int key_length = num_of_chars(argv[2]);
  if(dec_length > key_length) {
    memset(err_msg,'\0',sizeof(err_msg));
    sprintf(err_msg,"Error: key %s is too short",argv[2]);
    error(err_msg);
  }

  /* Set up the server address struct */
  /* Clear out the address struct */
  memset((char*)&serverAddress, '\0', sizeof(serverAddress)); 

  /* Get the port number, convert to an integer from a string */
  portNumber = atoi(argv[3]); 

  /* Create a network-capable socket */
  serverAddress.sin_family = AF_INET; 

  /* Store the port number */
  serverAddress.sin_port = htons(portNumber); 

  /* Using localhost as per assignment specification */
  serverHostInfo = gethostbyname("localhost"); 
  if(serverHostInfo == NULL) { 
      fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
      exit(0); 
  }

  /* Copy in the address */
  memcpy((char*)&serverAddress.sin_addr.s_addr, 
         (char*)serverHostInfo->h_addr, serverHostInfo->h_length); 

  /* Set up the socket */
  socketFD = socket(AF_INET, SOCK_STREAM, 0); /* Create the socket */
  if(socketFD < 0) error("CLIENT: ERROR opening socket");
  
  /* Connect to server */
  if(connect(socketFD,
      (struct sockaddr*)&serverAddress,sizeof(serverAddress))<0) 
      /* Connect socket to address */
      error("CLIENT: ERROR connecting");

  /* otp_dec_d.c will look for this message to be sent over */
  char handshake[SIZE];
  memset(handshake,'\0',sizeof(handshake));
  char mini_buffer[] = "otp_dec";
  int i;
  for(i=0;i<7;i++) {
    handshake[i] = mini_buffer[i];
  }

  /* Clear out the buffer array */
  memset(buffer, '\0', sizeof(buffer));

  /* send "otp_dec" to otp_dec_d.c" */
  temp = 0;
  charsWritten = 0;
  while(charsWritten < sizeof(handshake)) {
    temp = send(socketFD,handshake,sizeof(handshake),0);
    /* if temp ever equals -1 an error occurred */
    if(temp==-1) {
      error("Handshake send call failed in otp_dec.c");
    }
    charsWritten += temp;
  }

  /* if we receive "Handshake success" then we had successful handshake */
  temp = 0;
  charsRead = 0;
  memset(handshake,'\0',sizeof(handshake));
  while(charsRead < sizeof(handshake)) {
    temp = recv(socketFD,handshake,sizeof(handshake),0);
    /* if temp ever equals -1 an error occurred */
    if(temp==-1) {
      error("Handshake recv call failed in otp_dec.c");
    }
    /* store in buffer */
    for(i=charsRead;i < (charsRead + temp); i++) {
      buffer[i] = handshake[i];
    }
    charsRead += temp;
  }
  /* put buffer back in handshake */
  for(i=0;i<sizeof(buffer);i++) {
    handshake[i] = buffer[i];
  }
  memset(buffer, '\0', sizeof(buffer));

  /* if we receive anything besides "Handshake success" */
  if(strncmp(handshake,"Handshake success",17) != 0) {
    error("ERROR! Handshake failed");
  }
  
  /* connect and send communications */
  connector(argv[1],socketFD,dec_length);
  connector(argv[2],socketFD,key_length);

  /* receive decrypted file back */
  charsRead = 0;
  temp = 0;
  memset(buffer,'\0',sizeof(buffer));
  memset(temp_buffer,'\0',sizeof(temp_buffer));
  while(charsRead < sizeof(buffer)) {
    temp = recv(socketFD,buffer,sizeof(buffer),0);
    /* if temp ever equals -1 an error occurred */
    if(temp==-1) {
      error("otp_dec failed to read in decrypted file");
    }
    /* store in buffer */
    for(i=charsRead;i < (charsRead + temp); i++) {
      temp_buffer[i] = buffer[i];
    }
    charsRead += temp;
  }
  /* put buffer back in handshake */
  for(i=0;i<sizeof(temp_buffer);i++) {
    buffer[i] = temp_buffer[i];
  }

  /* buffer now holds decrypted message send it to stdout */
  printf("%s\n",buffer);

  close(socketFD); /* Close the socket */
  return 0;
}

/* counts the number of characters in a file */
int num_of_chars(const char* filename) {

  /* will hold current character and number of characters */
  int character = 0;
  int char_count = 0;

  char err_msg[SIZE];
  memset(err_msg,'\0',sizeof(err_msg));

  /* try to open file */
  FILE* fptr = fopen(filename,"r");
  if(fptr == NULL) {
    sprintf(err_msg,"Could not open file: %s",filename);
    error(err_msg);
  }
  /* while not at end of file */
  while(character != EOF && character != '\n') {
    character = fgetc(fptr);
    /* if character is not an uppercase letter or space */
    if((character < 65 && character != 32) || character > 90) {
      /* and if we are not at the end of file, error out */
      if(character != EOF && character != '\n') {
        error("otp_dec error: input contains bad characters");
      }
    }
    char_count++;
  }
  /* close file and return count */
  fclose(fptr);
  return char_count;
}

/* reads in undecrypted message and sends it to otp_dec_d for decryption */
void connector(char* filename,int socketFD,int length) {

  /* open file passed in */
  FILE* fptr = fopen(filename,"r");

  /* holds messages being passed through otp_dec */
  char buffer[SIZE];
  memset(buffer,'\0',sizeof(buffer));
  char temp_buffer[SIZE];
  memset(temp_buffer,'\0',sizeof(temp_buffer));

  /* holds error messages */
  char err_msg[SIZE];
  memset(err_msg,'\0',sizeof(err_msg));

  /* holds message length as chars */
  char char_length[SIZE];
  memset(char_length,'\0',sizeof(err_msg));

  /* keeps track of bites read in */
  int charsRead = 0;
  /* keeps track of bites written out */
  int charsWritten = 0;
  /* holds the return value of each read/write call */
  int temp = 0;

  /* read in file to buffer */
  /* modified from: https://stackoverflow.com
   * /questions/2029103/correct-way-to-read-a-text-file-into-a-buffer-in-c */
  if(fptr != NULL) {
    size_t reader = fread(buffer,sizeof(char),sizeof(buffer),fptr);
    if(ferror(fptr) != 0) {
      error("Error reading in file");
    }
    fclose(fptr);
  }
  
  /* send over message length to otp_dec_d.c */
  charsWritten = 0;
  snprintf(char_length,sizeof(char_length),"%d",length);
  while(charsWritten < sizeof(char_length)) {
    temp = send(socketFD,char_length,sizeof(char_length),0);
    /* if temp ever equals -1 an error occurred */
    if(temp==-1) {
      error("otp_dec failed to send length array");
    }
    charsWritten += temp;
  }

  /* loop while sending message */
  charsWritten = 0;
  usleep(500000);
  while(charsWritten < sizeof(buffer)) {
    temp = send(socketFD,buffer,sizeof(buffer),0);
    /* if temp ever equals -1 an error occurred */
    if(temp==-1) {
      error("otp_dec failed to write message to otp_dec_d");
    }
    charsWritten += temp;
  }
  /* clear out buffer to be safe */
  memset(buffer,'\0',sizeof(buffer));
}














