#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>

int numbers_size = 10;
int* numbers;
int numbers_pointer = 0;

void flush_char_buff(char* buff, int buffsize){
	int number;
	int res = sscanf(buff, "%d", &number);
	if (res > 0){
		if (numbers_pointer >= numbers_size){
			numbers_size = numbers_size + numbers_size/2;
			numbers = realloc(numbers, numbers_size * sizeof(int));
			if (numbers == NULL){
		    	fprintf(stderr, "%s\n", "Error: can't allocate memory");
		    	exit(1);
			}
		}
		numbers[numbers_pointer] = number;
		numbers_pointer++;
	}
	for(int i=0; i<buffsize; i++){
		buff[i] = '\0';
	}
}

int compare (const void * a, const void * b){
   return ( *(int*)a - *(int*)b );
}

int main(int argc, char *argv[]) {
    int files_amount = argc-2;
    int fd[files_amount];
    char ch;
    int mynum;
    
    numbers  = malloc(numbers_size * sizeof(int));
    if (numbers == NULL){
    	fprintf(stderr, "%s\n", "Error: can't allocate memory");
    	exit(1);
    }

    int buffsize = (int) log10(UINT_MAX);
    char buff[buffsize];
    int buff_pointer = 0;

    for (int i = 0; i < files_amount; ++i){
        fd[i] = open(argv[i+1], O_RDONLY);
        if (fd[i] == -1){
        	fprintf(stderr, "%s\n", "Error: Unable to open file");
        	exit(3);
        }
        while(read(fd[i], &ch, 1)){
            if (ch == '-'){
            	if (buff_pointer > 0){
            		flush_char_buff(buff, buffsize);
            		buff_pointer = 0;
            	}
            	buff[buff_pointer] = ch;
            	buff_pointer++;
            } else if ('0' <= ch && ch <= '9'){
            	if (buff_pointer >= buffsize){
            		fprintf(stderr, "%s\n", "Error: Buffer size exceed");
            		exit(2);
            	}
            	buff[buff_pointer] = ch;
            	buff_pointer++;
            } else {
            	if (buff_pointer > 0){
            		flush_char_buff(buff, buffsize);
            		buff_pointer = 0;
            	}
            }
        }
        close(fd[i]);
		flush_char_buff(buff, buffsize);
        buff_pointer = 0;
    }

    qsort(numbers, numbers_pointer, sizeof(int), compare);
    
    int fdout = open(argv[argc-1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fdout < 0) {
        fprintf(stderr, "%s\n", "Error: can't open file for writing");
        exit(5);
    }
    for (int i=0; i<numbers_pointer; i++){
    	dprintf(fdout, "%d ", numbers[i]);
    }
    return 0;
}
