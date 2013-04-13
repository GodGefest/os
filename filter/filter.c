#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define STDIN 0
#define STDOUT 1

void print(int fd, char *str, int len) {
    int printed = 0;
    while (printed < len) {
        printed += write(fd, str + printed, 
                                       len - printed);
    }
    if (str[len - 1] != '\n') {
        write(fd, "\n", 1);
    }
}

int find_del(char d, char* buf, int count) {
    int i = 0;
    for (i = 0; i < count; i++) {
        if (buf[i] == d) {
            return i;
        }
    }
    return -1;
}

int run_cmd_on(char* _argv[], int to, int j) {
    int devnull = open("/dev/null", O_WRONLY);
    int pid;
    int stat_val;
    if(pid = fork()) {
        wait(&stat_val);
        if (WIFEXITED(stat_val) && (WEXITSTATUS(stat_val) == 0)) {
            print(STDOUT, _argv[j], to);
        }
        close(devnull);
    } else {
        execvp(_argv[0], _argv);
    }
    return 0;
}

int main(int argc, char* argv[]) 
{
    int opt;

    char flag;
    
    int buf_size = 4096;

    while ((opt = getopt(argc, argv, "nzb:")) != -1) 
    {
        switch (opt)
        {
            case 'n':
                flag = '\n';
                break;
            case 'z':
                flag = 0;
                break;
            case 'b':
                buf_size = atoi(optarg);
                break;
            default:
                break; 
        }
    }
    char* buffer = malloc(buf_size);
    int len = 0; 
    int read_res = 0;
    int j = 0;
    int eof = 0;
    char** _argv = malloc(sizeof(char*) * (argc - optind + 1));
    while (argv[j + optind][0] != '{' || argv[j + optind][1] != '}' || argv[j + optind][2] != 0) {
        j++;
    } 
    memcpy(_argv, argv + optind, sizeof(char * ) * j);
    if (optind + j + 1 < argc) {
        memcpy(_argv + j + 1, argv + optind + j + 1, argc - optind - j - 1); 
    }
    _argv[argc - optind] = NULL;
    
    while(!eof) {
        read_res = read(STDIN, buffer + len, buf_size - len);
        //EOF - ?
        if (read_res == 0) {
            eof = 1;
        }
        int d_pos = find_del(flag, buffer + len, read_res);
        len = len + read_res;
        if (d_pos != -1) {
            while (d_pos != -1) { 
                char * tmp = malloc(sizeof(char) * (d_pos + 1));
                memcpy(tmp, buffer, d_pos);
                _argv[j] = tmp;
                run_cmd_on(_argv, d_pos, j);
                free(tmp);
                memmove(buffer, buffer + d_pos + 1, len - d_pos - 1);
                len = len - d_pos - 1;
                d_pos = find_del(flag, buffer, len);
            }
        }
    }
    if (len > 0) {
        if (len + 1 >= buf_size) {
            return -3;
        }
        buffer[len + 1] = flag;
        char * tmp = malloc(len + 1 - 0);
        memcpy(tmp, buffer, len + 1);
        _argv[j] = tmp;
        run_cmd_on(_argv, len + 1, j);
        free(tmp);
    }
    free(_argv);
    free(buffer);
    return 0;
}
