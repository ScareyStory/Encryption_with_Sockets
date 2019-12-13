/*******************************************************************************
** Program Name: otp_enc_d.c
** Author:       Story Caplain
** Last Updated: 11/05/19
** Description:  This program will run in the background as a daemon.
**               Its main function is to perform the encoding of plaintext.
**               This program will listen on a particular port/socket.
**               Port/socket is assigned when program is first ran
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#define SIZE 100000

/* Error function used for reporting issues */
void error(const char *msg) { fprintf(stderr,"%s\n",msg); exit(1); } 

/* allows for message sending between client and server */
void connector(int);

/* encrypts the communication */
void encrypter(char*,char*,int);

int main(int argc, char *argv[])
{
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
    socklen_t sizeOfClientInfo;
    char buffer[256];
    struct sockaddr_in serverAddress, clientAddress;

    /* Check usage & args */
    if(argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } 

    /* Set up the address struct for this process (the server) */
    /* Clear out the address struct */
    memset((char *)&serverAddress, '\0', sizeof(serverAddress)); 

    /* Get the port number, convert to an integer from a string */
    portNumber = atoi(argv[1]); 

    /* Create a network-capable socket */
    serverAddress.sin_family = AF_INET; 

    /* Store the port number */
    serverAddress.sin_port = htons(portNumber); 

    /* Any address is allowed for connection to this process */
    serverAddress.sin_addr.s_addr = INADDR_ANY; 

    /* Set up the socket */
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); 

    /* Create the socket */
    if(listenSocketFD < 0) error("ERROR opening socket");

    /* Enable the socket to begin listening */
    /* Connect socket to port */
    if(bind(listenSocketFD, 
       (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
       error("ERROR on binding");
       exit(1);
    }

    /* Flip the socket on - it can now receive up to 5 connections */
    listen(listenSocketFD, 5); 

    /* loop until loop is killed manually */
    while(1) {

      /* Get the size of the address for the client that will connect */  
      sizeOfClientInfo = sizeof(clientAddress); 

      /* Accept */
      establishedConnectionFD = accept(listenSocketFD, 
          (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 

      /* hold the status of waitpid calls */
      int exit_status = -5;

      /* set spawnpid to -5 to start, bogus value */
      pid_t spawnpid = -5;

      /* fork it */
      spawnpid = fork();

      switch(spawnpid) {

        /* fork failed */
        case -1:
          error("Hull Breach!");
          break;

        /* child process, call connector function */
        case 0:
          /* pass in file descriptor as argument */
          connector(establishedConnectionFD);
          /* close socket */
          break; 

        /* parent process, wait for children to finish */
        default:
          waitpid(spawnpid,&exit_status,0);
          break;
      }
      close(establishedConnectionFD);
    }
    close(listenSocketFD);
    return 0; 
}

/* read from socket */
void connector(int establishedConnectionFD) {

  /* init variables */
  int charsRead = 0, charsWritten = 0, temp = 0, i = 0;
  char buffer[SIZE];
  memset(buffer,'\0',sizeof(buffer));
  char temp_buffer[SIZE];
  memset(temp_buffer,'\0',sizeof(temp_buffer));
  char handshake[SIZE];
  memset(handshake,'\0',sizeof(handshake));
  char message[SIZE];
  memset(message,'\0',sizeof(message));
  char key[SIZE];
  memset(key,'\0',sizeof(key));

  /* fill buffer with stream opened from socket */
  while(charsRead < sizeof(buffer)) {
    temp = recv(establishedConnectionFD,buffer,sizeof(buffer),0);
    /* if temp ever equals -1 an error occurred */
    if(temp==-1) {
      error("Handshake failed in otp_enc_d.c | recv returned -1");
    }
    /* store in buffer */
    for(i=charsRead;i < (charsRead + temp); i++) {
      temp_buffer[i] = buffer[i];
    }
    charsRead += temp;
  }
  /* put temp buffer in buffer */
  for(i=0;i<sizeof(temp_buffer);i++) {
    buffer[i] = temp_buffer[i];
  }

  /* set up failure notice */
  char failure[] = "Handshake failure";
  charsWritten = 0;

  /* if otp_dec tries to use otp_enc_d inform of error */
  if(strncmp(buffer,"otp_dec",7)==0) {
    memset(buffer,'\0',sizeof(buffer));
    for(i=0;i<17;i++) {
      buffer[i] = failure[i];
    }
    while(charsWritten < sizeof(buffer)) {
      temp = send(establishedConnectionFD,buffer,sizeof(buffer),0);
      /* if temp ever equals -1 an error occurred */
      if(temp==-1) {
        error("Failed to inform of bad otp_dec send | send returned -1");
      }
      charsWritten += temp;
    }
    error("otp_dec cannot use otp_enc_d\n");
  }

  /* else if something weird happened */
  else if(strncmp(buffer,"otp_enc",7)!=0) {
    memset(buffer,'\0',sizeof(buffer));
    for(i=0;i<17;i++) {
      buffer[i] = failure[i];
    }
    while(charsWritten < sizeof(buffer)) {
      temp = send(establishedConnectionFD,buffer,sizeof(buffer),0);
      /* if temp ever equals -1 an error occurred */
      if(temp==-1) {
        error("Failed to inform of bad input | send returned -1");
      }
      charsWritten += temp;
    }
    error("otp_enc_d received bad input as handshake...\n");
  }
    
  /* else if handshake successful */
  else {

    /* reset buffer */
    memset(buffer,'\0',sizeof(buffer));

    /* put handshake success in buffer */
    int i;
    char mini_buffer[] = "Handshake success";
    for(i=0;i<17;i++) {
      buffer[i] = mini_buffer[i];
    }

    charsWritten = 0;
    while(charsWritten < sizeof(buffer)) {
      temp = send(establishedConnectionFD,buffer,sizeof(buffer),0);
      /* if temp ever equals -1 an error occurred */
      if(temp==-1) {
        error("Failed to send handshake in otp_enc_d.c | send returned -1");
      }
      charsWritten += temp;
    }

    /* reset buffer and charsRead for recv call */
    memset(buffer,'\0',sizeof(buffer));
    memset(temp_buffer,'\0',sizeof(temp_buffer));
    charsRead = 0;
    
    /* receive message text length into buffer*/
    while(charsRead < sizeof(buffer)) {
      temp = recv(establishedConnectionFD,buffer,sizeof(buffer),0);
      /* if temp ever equals -1 an error occurred */
      if(temp==-1) {
        error("Msg length recv failed in otp_enc_d.c | recv returned -1");
      }
      /* store in buffer */
      for(i=charsRead;i < (charsRead + temp); i++) {
        temp_buffer[i] = buffer[i];
      }
      charsRead += temp;
    }
    /* put temp buffer in buffer */
    for(i=0;i<sizeof(temp_buffer);i++) {
      buffer[i] = temp_buffer[i];
    }

    /* buffer holds length of message that will be sent over, atoi it */
    int msg_length = atoi(buffer);
    memset(buffer,'\0',sizeof(buffer));
    memset(temp_buffer,'\0',sizeof(temp_buffer));
    charsRead = 0;

    /* read in message sent through connection */
    while(charsRead < sizeof(buffer)) {
      temp = recv(establishedConnectionFD,buffer,sizeof(buffer),0);
      /* if temp ever equals -1 an error occurred */
      if(temp==-1) {
        error("Msg recv failed in otp_enc_d.c | recv returned -1");
      }
      /* store in buffer */
      for(i=charsRead;i < (charsRead + temp); i++) {
        temp_buffer[i] = buffer[i];
      }
      charsRead += temp;
    }
    /* put temp buffer in buffer */
    for(i=0;i<sizeof(temp_buffer);i++) {
      buffer[i] = temp_buffer[i];
    }

    /* put buffer into message */
    for(i=0;i<sizeof(buffer);i++) {
      message[i] = buffer[i];
    }
    charsRead = 0;
    
    /* receive key text length into buffer*/
    memset(buffer,'\0',sizeof(buffer));
    memset(temp_buffer,'\0',sizeof(temp_buffer));
    while(charsRead < sizeof(buffer)) {
      temp = recv(establishedConnectionFD,buffer,sizeof(buffer),0);
      /* if temp ever equals -1 an error occurred */
      if(temp==-1) {
        error("key length recv failed in otp_enc_d.c | recv returned -1");
      }
      /* store in buffer */
      for(i=charsRead;i < (charsRead + temp); i++) {
        temp_buffer[i] = buffer[i];
      }
      charsRead += temp;
    }
    /* put temp buffer in buffer */
    for(i=0;i<sizeof(temp_buffer);i++) {
      buffer[i] = temp_buffer[i];
    }

    /* buffer holds length of key that will be sent over, atoi it */
    int key_length = atoi(buffer);
    memset(buffer,'\0',sizeof(buffer));
    memset(temp_buffer,'\0',sizeof(temp_buffer));
    charsRead = 0;

    /* read in key sent through connection */
    while(charsRead < sizeof(buffer)) {
      temp = recv(establishedConnectionFD,buffer,sizeof(buffer),0);
      /* if temp ever equals -1 an error occurred */
      if(temp==-1) {
        error("key recv failed in otp_enc_d.c | recv returned -1");
      }
      /* store in buffer */
      for(i=charsRead;i < (charsRead + temp); i++) {
        temp_buffer[i] = buffer[i];
      }
      charsRead += temp;
    }
    /* put temp buffer in buffer */
    for(i=0;i<sizeof(temp_buffer);i++) {
      buffer[i] = temp_buffer[i];
    }

    /* put buffer into message */
    for(i=0;i<sizeof(buffer);i++) {
      key[i] = buffer[i];
    }
    memset(buffer,'\0',sizeof(buffer));
    memset(temp_buffer,'\0',sizeof(temp_buffer));
    charsRead = 0;

    /* encrypt communication */ 
    encrypter(message,key,msg_length);

    /* send encrypted message */
    charsWritten = 0;
    while(charsWritten < sizeof(message)) {
      temp = send(establishedConnectionFD,message,sizeof(message),0);
      /* if temp ever equals -1 an error occurred */
      if(temp==-1) {
        error("Failed to send encrypted message | send returned -1");
      }
      charsWritten += temp;
    }
    charsWritten = 0;
    charsRead = 0;
  }
}

void encrypter(char* message, char* key, int length) {

  int i; /* iterator */
  int m; /* holds msg char converted to num */
  int k; /* holds key char converted to num */ 

  /* for each character of the message */
  for(i=0;i<length;i++) {
    /* turn newline to null */
    if(message[i] == '\n'){message[i] = '\0';} 
    /* if not a newline */
    else { 
      /* convert each char in message to an int */
      if(message[i] == ' ') m = 26;
      else {m = message[i] - 'A';}
  
      /* convert each char in key to an int */
      if(key[i] == ' ') k = 26;
      else {k = key[i] - 'A';}
  
      /* add message char to key char 
       * and module by 27 (alphabet + ' ' = 27) */
      int encrypted = (m + k) % 27;
  
      /* store each encrypted char back into message */
      const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
      message[i] = alphabet[encrypted];
    }
  }
  /* ensure that message is null terminated */
  message[i] = '\0';
}















