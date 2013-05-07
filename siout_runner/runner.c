#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define STDIN 0
#define STDOUT 1

int find_new_string(char* str, int len) 
{
    int i;
    for (i = 1; i < len; i++)
    {
        if(str[i - 1] == '\0' && str[i] == '\0')
        {
            return i;
        }
    }
    return -1;
}

int find_new_word(char* str, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        if (str[i] == '\0') 
        {
            return i;
        }
    }
    return -1;
}

int cmd_exec(char* input, char** cmd, char* output)
{
    int pid;
    if (pid = fork())
    {

    }
    else 
    {
        int fdin = open(input, O_RDONLY);
        if (fdin == -1)
        {
            return -1;
        }
        dup2(fdin, STDIN);
        close(fdin);
        int fdout = open(output, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        dup2(fdout, STDOUT);
        close(fdout);
        execvp(cmd[0], cmd);
    }
    return 0;
}

int eat_string(char* str, int len)
{
    
    char* input;
    char* output;
    char** cmd = (char**) malloc((sizeof(char*) * 100));
    int count = 0;
    int d_pos = 0;
    int start_of_new_word = 0;
    input = str;
    start_of_new_word = find_new_word(str, len) + 1;
    while ((d_pos = 
            (find_new_word(str + start_of_new_word, len - start_of_new_word) 
                + start_of_new_word)) 
            != (len - 1))
    {
        cmd[count] = str + start_of_new_word; 
        count++;
        start_of_new_word = d_pos + 1;
    }
    if (count == 0)
    {
        free(cmd);
        return -1;
    }
    cmd[count] = NULL;
    output = str + start_of_new_word;
    if (cmd_exec(input, cmd, output))
    {
        free(cmd);
        return -1;
    }
    free(cmd);
    return 0;
}

int read_from_source(const int fd) 
{
    const int buf_size = 4096;
    char* buf = (char*) malloc(buf_size);
    int len = 0;
    int read_res = 0; 
    int d_pos = 0;
    int eof = 0;
    while (!eof)
    {
        read_res = read(fd, buf + len, buf_size - len);
        if (!read_res)
        {
            eof = 1;
        }
        d_pos = find_new_string(buf + len, read_res) + len;
        len = len + read_res;
        while (d_pos != -1) 
        {
            if (eat_string(buf, d_pos))
            {
                free(buf);
                return -1;
            } 
            memmove(buf, buf + d_pos +1, sizeof(char) * (len - d_pos -1));
            len = len - d_pos - 1;
            d_pos = find_new_string(buf, len);
        }
    }
    free(buf);
    return 0;
}

int main(int argc, char** argv) {
    char* source;
    int source_fd;

    if (argc != 2)
    {
        return -1;
    }

    source = argv[1];
    source_fd = open(source, O_RDONLY);
    if (read_from_source(source_fd))
    {
        return -1;
    }
    return 0;
}
