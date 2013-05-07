#include <unistd.h>
#include <fcntl.h>

#define STDIN 0
#define STDOUT 1

int read_from_source(const int fd) 
{
    const int buf_size = 4096;
    char* buf = malloc(buf_size);
    int len;
    int read_res; 
    int d_pos;
    int eof;
    while (!eof)
    {
        read_res = read(fd, buf + len, buf_size - len);
        if (!read_res)
        {
            eof = 1;
        }
        d_pos = find_del(buf + len, read_res);
        len = len + read_res;
        while (d_pos != -1) 
        {
            
        }

    }
}

int main(int argc, char** argv) {
    char* source;
    int source_fd;

    if (argc != 1)
    {
        return -1;
    }

    source = argv[1];
    source_fd = open(source, O_RDONLY);
    read_from_source(source_fd);
    return 0;
}
