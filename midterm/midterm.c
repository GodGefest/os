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

int _strlen(char* str) {
    int len = 0;
    while (str[len] != 0) {
       ++len;
    }
    return len; 
}

int find_places(char* str, int where[]) {
    int len = _strlen(str); 
    int i = 0;
    int count = 0;
    while (i < len - 1) {
        if (str[i] == '{' && str[i + 1] == '}') {
            where[count++] = i;
        }
        ++i;
    }
    return count;
} 

// Return value: 0 - ok, 1 - not ok;
int check_for_result(char* str, int len, char* cmd, int where[], int where_size) {
    if (len == 0) {
        len = 1;
        str[0] = 0;
        str[1] = 0;
        str[2] = 0;
    }
    const int max_cmd_len = 2048;
    char* tmp = malloc(max_cmd_len);
    int i;
    where[where_size] = _strlen(cmd);
    memcpy(tmp, cmd, where[0]);
    int cur_pos = where[0];
    for (i = 0; i < where_size; i++) {
        memcpy(tmp + cur_pos, str, len);
        cur_pos += len;
        memcpy(tmp + cur_pos, cmd + where[i] + 2, where[i + 1] - where[i] - 2);
        cur_pos += where[i + 1] - where[i] - 2; 
    }
    tmp[cur_pos] = 0;

    char** _tmp = malloc(1000);
    i = 0;
    int space_pos = find_del(' ', tmp, cur_pos);
    while ((space_pos = find_del(' ', tmp, cur_pos)) != -1) {
        _tmp[i] = malloc(space_pos + 1);
        memcpy(_tmp[i], tmp, space_pos);
        _tmp[i][space_pos] = 0;
        memmove(tmp, tmp + space_pos + 1, cur_pos -= space_pos);  
        i++;
    } 
    _tmp[i] = malloc(cur_pos + 1);
    memcpy(_tmp[i], tmp, cur_pos);
    _tmp[i][cur_pos] = 0;
    _tmp[i + 1] = NULL;
    int devnull = open("/dev/null", O_WRONLY);
    int pid;
    int stat_val;
    int ans = 1;
    if (pid = fork()) {
        wait(&stat_val);
        if (WIFEXITED(stat_val) && (WEXITSTATUS(stat_val) == 0)) {
           ans = 0;
        }
        close(devnull);
    } else {
        dup2(devnull, 1);
        execvp(_tmp[0], _tmp);
        exit(1);
    }
    i = 0;
    while (_tmp[i] != NULL) {
        free(_tmp[i]);
        ++i;
    }
    free(_tmp);
    free(tmp);
    if (ans) {
        return 1;
    } else {
        return 0;
    }
}

void eat_string(char* str, int len, int position, char* cmd, 
        int where[], int where_size) { 
    int count = 1;
    int begin = 0; // begin - first symbol of token, end - first symbol after.
    int end = 0;
    while (count != position && begin < len) {
        if (str[begin] == ' ') {
           ++count;
        }
        ++begin;
    }
    if (begin != len) {
        end = begin;
        while (str[end] != ' ' && end < len) {
            ++end;    
        }    
        char* temp = malloc(_strlen(str));    
        memcpy(temp, str, len);
        if (!check_for_result(temp + begin, end - begin, cmd, where, where_size)) {
            print(STDOUT, str, len);
        }
        free(temp);
    }
}

int main(int argc, char* argv[]) 
{
    if (argc < 3) {
        return -1;
    }

    int position = atoi(argv[1]);
    int where[100];
    int where_size = find_places(argv[2], where);


    const char del = '\n';
    const int buf_size = 4096;

    char* buffer = malloc(buf_size);
    int len = 0; 
    int read_res = 0;
    int eof = 0;
    
    while(!eof) {
        read_res = read(STDIN, buffer + len, buf_size - len);
        if (read_res == 0) {
            eof = 1;
        }
        int d_pos = find_del(del, buffer + len, read_res) + len;
        len = len + read_res;
        if (d_pos != -1) {
            while (d_pos != -1) { 
                eat_string(buffer, d_pos, position, argv[2], where, where_size);
                memmove(buffer, buffer + d_pos + 1, sizeof(char) * (len - d_pos - 1));
                len = len - d_pos - 1;
                d_pos = find_del(del, buffer, len);
            }
        }
    }
   
    free(buffer);
    return 0;
}
