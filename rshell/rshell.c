#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define MAX_COUNT_OF_CONNECTIONS 5

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

int main()
{

    struct addrinfo hints; 
    struct addrinfo *result;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int res_getaddrinfo = getaddrinfo(NULL, "8822", &hints, &result);
    if (res_getaddrinfo != 0)
    {
        const char *error = gai_strerror(res_getaddrinfo);
        my_write(STDERR, error, strlen(error)); 
        exit(EXIT_FAILURE);
    }

    int sfd;
    struct addrinfo *it; 
    for (it = result; it != 0; it = it->ai_next)
    {
        sfd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (sfd == -1)
        {
            continue;
        }

        int opt_val = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
        
        if (bind(sfd, it->ai_addr, it->ai_addrlen) == 0)
        {
            break;
        }
        close(sfd);
    } 

    if (it == NULL)
    {
        const char *error = "Coudln't bind\n";
        my_write(STDERR, error, strlen(error));
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);
    
    int listen_res = listen(sfd, MAX_COUNT_OF_CONNECTIONS);
    if (listen_res == -1) 
    {
        perror("Listen failed");
    }

    struct sockaddr address;
    socklen_t address_len = sizeof(address);

    while (true)
    {
        int cfd = accept(sfd, &address, &address_len); 
        
        int pid = fork();
        if (pid)
        {
            close(cfd);
        }
        else 
        {
            dup2(cfd, STDIN);
            dup2(cfd, STDOUT);
            dup2(cfd, STDERR);
            close(sfd);
            const char *message = "Hello world!\n";
            my_write(cfd, message, strlen(message));
            close(cfd);
            exit(0);
        }

    }

    close(sfd);
    return 0;
}
