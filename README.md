# Encryption_with_Sockets 
(Description paraphrased from Prof. Brewster's from Operating Systems 1 at Oregon State University) 
This program uses localhost as the target IP address/host

EXAMPLE RUN AT BOTTOM OF README

After moving to the folder you have put the files in, run:

compileall

This compiles all 5 of the .c files using gcc.

# otp_enc_d
otp_enc_d: This program will run in the background as a daemon. Upon execution, otp_enc_d outputs an error if it cannot be run due to a network error, such as the ports being unavailable. Its function is to perform the actual encoding. This program will listen on a particular port/socket, assigned when it is first ran (see syntax below). 

The child process of otp_enc_d must first check to make sure it is communicating with otp_enc (see otp_enc, below). After verifying that the connection to otp_enc_d is coming from otp_enc, then this child receives from otp_enc plaintext and a key via the communication socket (not the original listen socket). The otp_enc_d child will then write back the ciphertext to the otp_enc process that it is connected to via the same communication socket. 

Note that the key passed in must be at least as big as the plaintext!

otp_enc_d supports up to five concurrent socket connections running at the same time; this is different than the number of processes that could queue up on your listening socket. Again, only in the child process will the actual encryption take place, and the ciphertext be written back: the original server daemon process continues listening for new connections, not encrypting data.

Use this syntax for otp_enc_d:

otp_enc_d <listening_port> &

listening_port is the port that otp_enc_d should listen on. You will always start otp_enc_d in the background, as follows (the port 57171 is just an example; yours should be able to use any port):

$ otp_enc_d 57171 &

# otp_enc
otp_enc: This program connects to otp_enc_d, and asks it to perform a one-time pad style encryption as detailed above. By itself, otp_enc doesn’t do the encryption - otp_enc_d does. The syntax of otp_enc is as follows:

otp_enc plaintext <key> <port>
In this syntax, plaintext is the name of a file in the current directory that contains the plaintext you wish to encrypt. Similarly, key contains the encryption key you wish to use to encrypt the text. Finally, port is the port that otp_enc should attempt to connect to otp_enc_d on.

When otp_enc receives the ciphertext back from otp_enc_d, it outputs it to stdout. Thus, otp_enc can be launched in any of the following methods, and should send its output appropriately:

$ otp_enc myplaintext mykey 57171
$ otp_enc myplaintext mykey 57171 > myciphertext
$ otp_enc myplaintext mykey 57171 > myciphertext &

If otp_enc receives key or plaintext files with ANY bad characters in them, or the key file is shorter than the plaintext,  it  terminates, sends appropriate error text to stderr, and sets the exit value to 1.

otp_enc cannot connect to otp_dec_d, even if it tries to connect on the correct port.

# otp_dec_d
otp_dec_d: This program performs exactly like otp_enc_d, in syntax and usage. In this case, however, otp_dec_d will decrypt ciphertext it is given, using the passed-in ciphertext and key. Thus, it returns plaintext again to otp_dec.

# otp_dec
otp_dec: Similarly, this program will connect to otp_dec_d and will ask it to decrypt ciphertext using a passed-in ciphertext and key, and otherwise performs exactly like otp_enc, and is runnable in the same three ways. otp_dec cannot connect to otp_enc_d, even if it tries to connect on the correct port.

# keygen
keygen: This program creates a key file of specified length.

The syntax for keygen is as follows:

keygen <keylength>
Where keylength is the length of the key file in characters. keygen outputs to stdout. Here is an example run, which redirects stdout to a key file of 256 characters called “mykey” (note that mykey is 257 characters long because of the newline):

$ keygen 256 > mykey

# Example

Here is an example of usage, if you were testing your code from the command line:

$ compileall

$ cat plaintext1

THE RED GOOSE FLIES AT MIDNIGHT STOP

$ otp_enc_d 57171 &

$ otp_dec_d 57172 &

$ keygen 10

EONHQCKQ I

$ keygen 10 > mykey

$ cat mykey

VAONWOYVXP

$ keygen 10 > myshortkey

$ otp_enc plaintext1 myshortkey 57171 > ciphertext1 

Error: key ‘myshortkey’ is too short

$ echo $?

1

$ keygen 1024 > mykey

$ otp_enc plaintext1 mykey 57171 > ciphertext1

$ cat ciphertext1

WANAWTRLFTH RAAQGZSOHCTYS JDBEGYZQDQ

$ keygen 1024 > mykey2

$ otp_dec ciphertext1 mykey 57172 > plaintext1_a

$ otp_dec ciphertext1 mykey2 57172 > plaintext1_b

$ cat plaintext1_a

THE RED GOOSE FLIES AT MIDNIGHT STOP

$ cat plaintext1_b

WSXFHCJAEISWQRNO L ZAGDIAUAL IGGTKBW

$ cmp plaintext1 plaintext1_a

$ echo $?

0

$ cmp plaintext1 plaintext1_b

plaintext1 plaintext1_b differ: byte 1, line 1

$ echo $?

1

$ otp_enc plaintext5 mykey 57171

otp_enc error: input contains bad characters

$ echo $?

1

$ otp_enc plaintext3 mykey 57172

Error: could not contact otp_enc_d on port 57172

$ echo $?

1

$
