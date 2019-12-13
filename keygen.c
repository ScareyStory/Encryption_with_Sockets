#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(int argc, char *argv[]) {

    /* if user does not pass in length as argument */
    if(argc < 2) {
        fprintf(stderr, "Error! keygen takes length as an argument\n");
    }

    /* seed random number generator */
    srand(time(NULL));

    /* convert length string to an integer */
    int length = atoi(argv[1]);

    /* store alphabet + space in array */
    char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    /* init random_letter for indexing alphabet array, and init iterator */
    int random_letter, i;

    /* loop length times and write each letter to stdout */
    for(i = 0; i < length; i++) {
        random_letter = alphabet[rand() % 27];
        fprintf(stdout,"%c",random_letter);
    }
    /* add newline */
    fprintf(stdout,"%c",'\n');
    return 0;
}
