#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <pty.h>
#include <poll.h>
#include <string>
#include <vector>
#include <map>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define BUF_SIZE 4096
#define MAX_COUNT_OF_CONNECTIONS 100

using namespace std;

int my_write(int fd, const char *str, int len)
{
    int written = 0;
    int write_ret = 0;
    while (written < len)
    {
        write_ret = write(fd, str + written, len - written);
        if (write_ret < 0)
        {
            return -1;
        }
        written += write_ret; 
    }
    return 0;
}

int my_read(int fd, char *buf, int len)
{
    int readed = read(fd, buf, len);
    if (readed < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    return readed;
}

void *my_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == 0)
    {
        exit(EXIT_FAILURE);
    }
    return ptr;
}

int my_poll(pollfd fd[], int nfds)
{
    int ans = poll(fd, nfds, -1);
    if (ans == -1)
    {
        perror("poll");
        exit(EXIT_FAILURE);
    }
    return ans;
}

struct addr {
    string ip;
    string port;

    addr(string ip, string port) : ip(ip), port(port) {}

};


int main(int argc, char* argv[])
{

    int clients = 3;
    pollfd fd[MAX_COUNT_OF_CONNECTIONS + 3];

    string port(argv[1]); 
    vector<addr> css;
    map<string, <vector<string> > > m;
    vector<char*> bufs(3);

    for (int i = 2; i < argc; ++i)
    {
        string s(argv[i]);
        size_t col = s.find(':');
        if (col == string::npos)
        {
            perror("Wrong arg");
        }
        css.push_back(addr(s.substr(0, col), s.substr(col + 1, s.size() - col - 1)));
    }

    struct addrinfo hints_c;
    struct addrinfo *result_c;

    for (size_t i = 0; i < css.size(); ++i)
    {
        memset(&hints_c, 0, sizeof(struct addrinfo));
        int cfd;
        hints_c.ai_family = AF_INET;
        hints_c.ai_socktype = SOCK_STREAM;
        hints_c.ai_family = 0;
        hints_c.ai_protocol = 0;

        int res_getaddrinfo_c = 
            getaddrinfo(css[i].ip.c_str(), css[i].port.c_str(), &hints_c, &result_c);
        if (res_getaddrinfo_c != 0)
        {
            perror("getaddrinfo_c fails");
            exit(EXIT_FAILURE);
        }
        cfd = socket(result_c->ai_family, result_c->ai_socktype, result_c->ai_protocol);
        if (connect(cfd, result_c->ai_addr, result_c->ai_addrlen) == -1)
        {
            perror("connect failed");
            exit(EXIT_FAILURE);
        }
        perror("connected");
        clients++;
        fd[i + 3].fd = cfd;
        fd[i + 3].events = POLLIN;
        bufs.push_back((char *) malloc(BUF_SIZE));

        freeaddrinfo(result_c);
    }
    struct addrinfo hints_s; 
    struct addrinfo *result_s;

    memset(&hints_s, 0, sizeof(struct addrinfo));
    hints_s.ai_family = AF_INET;
    hints_s.ai_socktype = SOCK_STREAM;
    hints_s.ai_flags = AI_PASSIVE;
    hints_s.ai_protocol = 0;
    hints_s.ai_canonname = NULL;
    hints_s.ai_addr = NULL;
    hints_s.ai_next = NULL;

    int res_getaddrinfo_s = getaddrinfo(NULL, port.c_str(), &hints_s, &result_s);
    if (res_getaddrinfo_s != 0)
    {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    int sfd = socket(result_s->ai_family, result_s->ai_socktype, result_s->ai_protocol);

    int opt_val = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
    
    if (bind(sfd, result_s->ai_addr, result_s->ai_addrlen) == -1)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result_s);
    
    if (listen(sfd, MAX_COUNT_OF_CONNECTIONS) == -1)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    fd[0].fd = STDIN;
    fd[0].events = POLLIN;
    fd[1].fd = STDOUT;
    fd[1].events = POLLOUT;
    fd[2].fd = sfd;
    fd[2].events = POLLIN;

    struct sockaddr address;
    socklen_t address_len = sizeof(address);

    
    while (true)
    {
        my_poll(fd, clients);
        for (int i = 3; i < clients; i++)
        {
            if (fd[i].revents & (POLLERR | POLLHUP))
            {
                fd[i] = fd[clients - 1];
                fd[i].events = fd[clients - 1].events;
                clients--;
                continue;
            }
            if (fd[i].revents & POLLIN)
            {
                 
            }
        }
        if (fd[2].revents && POLLIN)
        {
            int cfd = accept(sfd, &address, &address_len); 
            if (cfd == -1)
            {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
            perror("accepted");
            fd[clients].fd = cfd;
            fd[clients].events = POLLIN;
            clients++;
        }
    }
    close(sfd);
    return 0;
}
