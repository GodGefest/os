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

void find_range(char* str, int* from, int* to) {
    int len = _strlen(str); 
    char* tmp = malloc (len);
    int i = 0;
    while (str[i] != '-') {
        ++i;
    }
    memmove(tmp, str, i);
    tmp[i] = 0;
    *from = atoi(tmp);
    memmove(tmp, str + i + 1, len - i - 1);
    tmp[len - i - 1] = 0;
    *to = atoi(tmp); 
    free(tmp);
} 

// Return value: 0 - ok, 1 - not ok;
int check_for_range(char* str, int len, int from, int to) {
    int i;
    if (str[0] == '-') {
        i = 1;
    } else {
        i = 0;
    }
    for (; i < len; ++i) {
        if (str[i] < '0' || str[i] > '9') {
            return 1;
        }
    }
    char* tmp = malloc(len + 1);
    memmove(tmp, str, len);
    tmp[len] = 0;
    int ans = atoi(tmp);
    free(tmp);
    if (ans < from || ans > to) {
        return 1;
    }
    return 0;
}

void eat_string(char* str, int len, int position, int from, int to) { 
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
        end = begin + 1;
        while (str[end] != ' ' && end < len) {
            ++end;    
        }    
        if (!check_for_range(str + begin, end - begin, from, to)) {
            print(STDOUT, str, len);
        }
    }
}

int main(int argc, char* argv[]) 
{
    if (argc < 3) {
        return -1;
    }

    int position = atoi(argv[1]);
    int from;
    int to;
    find_range(argv[2], &from, &to);


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
                eat_string(buffer, d_pos, position, from, to);
                memmove(buffer, buffer + d_pos + 1, sizeof(char) * (len - d_pos - 1));
                len = len - d_pos - 1;
                d_pos = find_del(del, buffer, len);
            }
        }
    }
   
    free(buffer);
    return 0;
}
