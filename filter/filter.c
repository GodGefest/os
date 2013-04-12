#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#define STDIN 0

int find_del(char d, char* buf, int count) {
    int i = 0;
    for (i = 0; i < count; i++) {
        if (buf[i] == d) {
            return i;
        }
    }
    return -1;
}

int run_cmd_on(char* buffer, char* argv[], int argc, int from, int to) {
    int i;
    char* _argv[argc - optind + 1];
    int j = 0;
   // int STDOUTcp = dup(1);
    int devnull = open("/dev/null", O_WRONLY);
    while (argv[j + optind][0] != '{' || argv[j + optind][1] != '}' || argv[j + optind][2] != 0) {
        j++;
    } 
    memcpy(_argv, argv + optind, sizeof(char * ) * j);
    char * tmp = malloc(to - from + 1);
    for (i = 0; i < to - from; i++) {
        tmp[i] = buffer[from + i];
    }
    tmp[to] = 0;
    _argv[j] = tmp;
    if (optind + j + 1 < argc) {
        memcpy(_argv + j + 1, argv + optind + j + 1, argc - optind - j - 1); 
    }
    _argv[argc - optind] = NULL;
    int pid;
    int stat_val;
    if(pid = fork()) {
        wait(&stat_val);
        if (WIFEXITED(stat_val) && (WEXITSTATUS(stat_val) == 0)) {
    //        dup2(STDOUTcp, 1);
            write(1, tmp, to);
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
    int i = 0;
    int eof = 0;
    int from;
    while(!eof) {
        read_res = read(STDIN, buffer + len, buf_size - len);
        //EOF - ?
        if (read_res == 0) {
            eof = 1;
        }
        int d_pos = find_del(flag, buffer + len, read_res);
        from = d_pos;
        len = len + read_res;
        if (d_pos != -1) {
            while (d_pos != -1) { 
                run_cmd_on(buffer, argv, argc, 0, d_pos);
                memmove(buffer, buffer + d_pos + 1, len - d_pos - 1);
                len = len - d_pos - 1;
                from = 0;
                d_pos = find_del(flag, buffer, len);
            }
        }
    }
    if (len > 0) {
        if (len + 1 >= buf_size) {
            return -3;
        }
        buffer[len + 1] = flag;
        run_cmd_on(buffer, argv, argc, 0, len + 1);
    }
    return 0;
}
