#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define STDIN 0
#define STDOUT 1

//return values: 0 - is ok, -1 - smth wrong
int _print (char* buffer, int from, int to){
    int from_after = from;
    int write_return = 0;
    while (from_after < to) {
        write_return = write(STDOUT, buffer + from_after, to - from_after);
        if (write < 0) {
            return -1;
        }
        from_after += write_return;
    }
    return 0;
}


int main(int argc, char** argv) {
    int k = 0;
    char* buffer = 0;
    int read_return = 0;
    int cur_busy = 0;
    int eof = 0;
    int ignoring = 0;
    int from = 0;
    int to = 0;
    int i;
    if (argc < 2) {
        return 1;
    }
    k = atoi(argv[1]);
    if (k < 1) {
        return 1;
    }
    buffer = malloc(k);
                                    printf("k = %d\n", k);
    while (1) {
        read_return = read(STDIN, buffer + cur_busy, k - cur_busy); 
                                    printf("read_return = %d\n", read_return);
        if (read_return == 0) {
            eof = 1;
        } else if (read_return < 0) {
            return 1;
        } else {
            from = 0;
            for (i = cur_busy; i < cur_busy + read_return; ++i) {
                printf("buffer[%d] = %d\n", i, buffer[i]);
                if (buffer[i] == '\n') {
                    if (ignoring) {
                        from = i + 1;
                        ignoring = 0; 
                    }
                    to = i + 1;
                    _print(buffer, from, to);
                    _print(buffer, from, to);
                }
            }
        }
        if (eof) {
            break;
        }
    }
    return 0;
}
