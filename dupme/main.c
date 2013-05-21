#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1

//return values: 0 - is ok, -1 - smth wrong
int _print (char* buffer, int from, int to){
    int from_after = from;
    int write_return = 0;
    while (from_after < to) {
        write_return = write(STDOUT, buffer + from_after, to - from_after);
        if (write_return < 0) {
            return -1;
        }
        from_after += write_return;
    }
    return 0;
}

int _read (char* arg) {
    int size = 0;
    int i = 0;
    while(arg[i] != 0) {
        size *= 10;
        size += arg[i] - 48;
        i++;
    }
    return size;
}

int k = 0;
char* buffer = 0;
int read_return = 0;
int used = 0;
int eof = 0;
int ignoring = 0;
int from = 0;
int to = 0;
int i;

int main(int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }
    k = _read(argv[1]); 
    if (k < 1) {
        return 1;
    }
    buffer = malloc(++k);
    while (1) {
        read_return = read(STDIN, buffer + used, k - used); 
        if (read_return == 0) {
            eof = 1;
            if (used > 0 && !ignoring) {
                buffer[used] = '\n';
                read_return = used + 1;
                used = 0;
            }
        } else if (read_return < 0) {
            return 1;
        }
        from = 0;
        for (i = used; i < used + read_return; ++i) {
            if (buffer[i] == '\n') {
                if (ignoring) {
                    from = i + 1;
                    ignoring = 0; 
                }
                to = i + 1;
                if (_print(buffer, from, to) < 0) {
                    return 1;
                };
                if (_print(buffer, from, to) < 0) {
                    return 1;
                };
                from = i + 1;
            }
        }
        used = used + read_return - from;
        if (eof) {
            break;
        }
        memmove(buffer, buffer + from, used);
        if (used == k) {
            ignoring = 1;
            used = 0;
        }
    }
    return 0;
}
