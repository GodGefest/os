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

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define MAX_COUNT_OF_CONNECTIONS 5

struct FD_pair 
{
    int fds[2];
    bool deads[2];
    bool closed;

    FD_pair(int in, int out)
    {
        fds[0] = in;
        fds[1] = out;
        deads[0] = false;
        deads[1] = false;
        closed = false;
    }

    bool both_dead()
    {
        return (deads[0] && deads[1]);
    }

};

void my_poll(int fd1, int fd2)
{ 
    int buf_size = 4096;
    char buf1to2[buf_size];
    int len1to2 = 0;
    char buf2to1[buf_size];
    int len2to1 = 0;
    int readed = 0;
    int written = 0;
    FD_pair fdp[2] = { FD_pair(fd1, fd2), FD_pair(fd2, fd1) };
    struct pollfd fds[2];
    fds[0].fd = fd1;
    fds[1].fd = fd2;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;
    while (!fdp[0].both_dead() && !fdp[1].both_dead()) 
    {
        fds[0].events = 0;
        fds[1].events = 1;
        int ret = poll(fds, 2, -1);
        if (ret > 0) 
        {
            for (int i = 0; i < 2; i++)
            {
                if ((fds[i].revents & POLLERR) || 
                        (fds[i].revents & POLLHUP))
                {
                    fdp[i ^ 1].deads[i] = true;
                    fdp[i].deads[i ^ 1] = true;
                    fdp[0].closed = true;
                    fdp[1].closed = true;
                }
            }
            if ((!fdp[0].deads[0]) && (len1to2 < buf_size) 
                    && (fds[0].revents & POLLIN) && (!fdp[0].closed))
            {
                readed = read(fd1, buf1to2 + len1to2, buf_size - len1to2);
                len1to2 += readed;
                if (readed == 0)
                {
                    fdp[0].deads[0] = true; 
                    fdp[0].closed = true;
                }
            }
            if ((!fdp[1].deads[1]) && (len2to1 > 0) && (fds[0].revents & POLLOUT))
            {
                written = write(fd1, buf2to1, len2to1);
                memmove(buf2to1, buf2to1 + written, len2to1 - written);
                len2to1 -= written;
            }
            if ((!fdp[1].deads[0]) && (len2to1 < buf_size) 
                    && (fds[1].revents & POLLIN) && (!fdp[0].closed))
            {
                readed = read(fd2, buf2to1 + len2to1, buf_size - len2to1);
                len2to1 += readed;
                if (readed == 0)
                {
                    fdp[1].deads[0] = true; 
                    fdp[1].closed = true;
                }
            }
            if ((!fdp[0].deads[1]) && (len1to2 > 0) && (fds[1].revents & POLLOUT))
            {
                written = write(fd2, buf1to2, len1to2);
                memmove(buf1to2, buf1to2 + written, len1to2 - written);
                len1to2 -= written;
            }
        if (fdp[0].deads[0] && len1to2 == 0)
            fdp[0].deads[1] = true;
        if (fdp[1].deads[0] && len2to1 == 0)
            fdp[1].deads[1] = true;

        if (!fdp[0].closed && len1to2 < buf_size)
            fds[0].events |= POLLIN;
        if (!fdp[1].closed && len2to1 < buf_size)
            fds[1].events |= POLLIN;
        if (len1to2 > 0 && !fdp[0].deads[1])
            fds[1].events |= POLLOUT; 
        if (len2to1 > 0 && !fdp[1].deads[1])
            fds[0].events |= POLLOUT;
        }

    }
    return;
}

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
    int main_pid = fork();
    if (main_pid)
    {
        wait();
    }
    else
    {
        setsid();

        struct addrinfo hints; 
        struct addrinfo *result;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
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

        struct addrinfo *it; 
        it = result;
        int sfd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);

        int opt_val = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
        
        if (bind(sfd, it->ai_addr, it->ai_addrlen) == -1)
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
            perror("accepted");
            int pid = fork();
            if (pid)
            {
                close(cfd);
            }
            else 
            {
                close(STDIN);
                close(STDOUT);
                close(STDERR);
                close(sfd);
                char buf[4096];
                int master, slave;
                openpty(&master, &slave, buf, NULL, NULL);
                if (fork())
                {
                    close(slave);
                    my_poll(master, cfd);
                    close(master);
                    close(cfd);
                    exit(1);
                }
                else
                {
                    setsid();
                    close(master);
                    dup2(slave, STDIN);
                    dup2(slave, STDOUT);
                    dup2(slave, STDERR);
                    close(cfd);
                    int fd = open(buf, O_RDWR);
                    close(fd);
                    execl("/bin/sh", "/bin/sh", NULL);
                }
            }

        }

        close(sfd);
    }
    return 0;
}
