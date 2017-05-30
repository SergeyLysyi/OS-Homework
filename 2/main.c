#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define ERRNO_UNABLE_TO_OPEN 1
#define MSG_UNABLE_TO_OPEN "Error: unable to open file"
#define ERRNO_UNABLE_TO_WRITE 2
#define MSG_UNABLE_TO_WRITE "Error: unable to write to file"
#define ERRNO_UNABLE_TO_SKIP 3
#define MSG_UNABLE_TO_SKIP "Error: unable to skip byte of file"
#define ERRNO_WRONG_ARGUMENTS 4
#define MSG_WRONG_ARGUMENTS "Error: wrong arguments"

#define MSG_HELP "Usage: gzip -cd sparse-file.gz | %s newsparsefile\n"

#define CH_HOLE '\0'

#define MOVE_DIRECTION_FORWARD 1
#define MOVE_DIRECTION_BACKWARD 2

void writeByte(int fd, const char c){
    if (write(fd, &c, 1) != 1) {
    	fprintf(stderr, "%s\n", MSG_UNABLE_TO_WRITE);
        exit(ERRNO_UNABLE_TO_WRITE);
    }
}

void moveInFile(int fd, int direction){
	int moveDirection;
	if (direction == MOVE_DIRECTION_BACKWARD){
		moveDirection = -1;
	} else {
		moveDirection = 1;
	}

    if (lseek(fd, moveDirection, SEEK_CUR) == -1) {
        fprintf(stderr, "%s\n", MSG_UNABLE_TO_SKIP);
        exit(ERRNO_UNABLE_TO_SKIP);
    }
}

int main(int argc, const char * argv[]) {
    int fdin = STDIN_FILENO;
    int fdout;
    char ch;

    if (argc != 2) {
    	fprintf(stderr, "%s\n", MSG_WRONG_ARGUMENTS);
        printf(MSG_HELP, argv[0]);
        return ERRNO_WRONG_ARGUMENTS;
    }
    
    fdout = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fdout < 0) {
        fprintf(stderr, "%s\n", MSG_UNABLE_TO_OPEN);
        return ERRNO_UNABLE_TO_OPEN;
    }
    
    while(read(fdin, &ch, 1) > 0) {
        if (ch == CH_HOLE) {
            moveInFile(fdout, MOVE_DIRECTION_FORWARD);
        } else {
        	writeByte(fdout, ch);
        }
    }

    // last byte is never hole byte
    if (ch == CH_HOLE) {
    	moveInFile(fdout, MOVE_DIRECTION_BACKWARD);
        writeByte(fdout, ch);
    }
    
    close(fdout);

    return 0;
}