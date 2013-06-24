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
#include <utility>
#include <set>
#include <algorithm>

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

struct mes {
    string key;
    string value;
    string new_value;
    string id;
};

int check_ready(const pair<char *, int> &p)
{
    for (int i = 0; i < p.second; ++i)
    {
        if (p.first[i] == '#')
        {
            return i;
        }
    }
    return -1;
}

bool is_print(const pair<char *, int> &p)
{
    if (p.second > 6)
    {
        if (p.first[0] == 'p' && p.first[1] == 'r' && p.first[2] == 'i' && 
                p.first[3] == 'n' && p.first[4] == 't' && p.first[5] == '\n')
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool check_correct(const pair<char *, int> &p, int len)
{
    int count_ats = 0;
    int count_lets = 0;
    for (int i = 0; i < len; ++i)
    {
        if (p.first[i] == '@')
        {
            count_ats++;
            if (count_lets == 0)
            {
                return false;
            }
            count_lets = 0;
        }
        else
        {
            count_lets++;
        }
    }
    if (count_lets > 0 && count_ats == 3)
    {
        return true;
    }
    else
    {
        return false;
    } 
}

mes parse(const pair<char *, int> &p, int del)
{
    int from = 0;
    int to = 0;
    int state = 0;
    mes m;
    for (int i = 0; i < del; i++)
    {
        if (p.first[i] == '@')
        {
            to = i;
            if (state == 0)
            {
                m.key = string(p.first[from], to - from);
            }
            if (state == 1)
            {
                m.value = string(p.first[from], to - from);
            }
            if (state == 2)
            {
                m.new_value = string(p.first[from], to - from);
            }
            if (state == 3)
            {
                m.id = string(p.first[from], to - from);
            }
            state++;
            from = i + 1;
        }
    }
    return m;
}

bool is_again(map<string, pair<vector<string>, 
        set<string> > >::iterator it, mes message)
{
    if ((it->second).second.find(message.id) != it->second.second.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool is_collision(map<string, pair<vector<string>, 
        set<string> > >::iterator it, mes message)
{
    vector<string> v = it->second.first;
    if (find(v.begin(), v.end(), message.value) != v.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

void add(map<string, pair<vector<string>, set<string> > > &m, mes message)
{
    m[message.value].first.push_back(message.new_value);
    m[message.value].second.insert(message.id);
}

string mes2str(mes message)
{
    return message.key + "@" + message.value + "@" + message.new_value + "@" 
        + message.id + "#";
}

void send2all(vector<pair<char*, int> > &bufs, string message, size_t j)
{
    for (size_t i = 4; i < bufs.size(); i += 2)
    {
        if (j != i)
        {
            for (size_t k = 0; k < message.size(); ++k)
            {
                bufs[i].first[bufs[i].second + k] = message[k];
            }
        }
    }
}

//format key@value@new_value@id#
// collision = $$$
int main(int argc, char* argv[])
{
    int id = 1;
    const string collision = "$$$";
    int clients = 3;
    pollfd fd[MAX_COUNT_OF_CONNECTIONS + 3];

    string port(argv[1]); 
    vector<addr> css;
    map<string, pair<vector<string>, set<string> > > m;
    vector<pair<char*, int> > bufs(3);

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
        clients += 2;
        fd[2 * i + 3].fd = cfd;
        fd[2 * i + 3].events = POLLIN;
        fd[2 * i + 4].fd = cfd;
        fd[2 * i + 4].events = POLLOUT;
        bufs.push_back(pair<char *, int>((char *) my_malloc(BUF_SIZE), 0));
        bufs.push_back(pair<char *, int>((char *) my_malloc(BUF_SIZE), 0));

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

    fd[1].fd = STDIN;
    fd[1].events = POLLIN;
    fd[2].fd = STDOUT;
    fd[2].events = POLLOUT;
    fd[0].fd = sfd;
    fd[0].events = POLLIN;

    struct sockaddr address;
    socklen_t address_len = sizeof(address);

    
    while (true)
    {
        my_poll(fd, clients);
        for (int i = 2; i < clients; i++)
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
                int len = my_read(fd[i].fd, bufs[i].first + bufs[i].second, 
                        BUF_SIZE - bufs[i].second);
                bufs[i].second += len;
                if (int delim = check_ready(bufs[i]) != -1)
                {
                    if (check_correct(bufs[i], delim))
                    {
                        mes message = parse(bufs[i], delim);
                        if (message.new_value == collision)
                        {
                            add(m, message);
                            string str = mes2str(message);
                            send2all(bufs, str, i);
                        }
                        else
                        { 
                            auto it = m.find(message.key);
                            if (it == m.end())
                            {
                                if (message.value == collision)
                                {
                                    pair<vector<string>, set<string> > p;
                                    p.first = vector<string>();
                                    p.second = set<string>();
                                    p.first.push_back(message.new_value);
                                    p.second.insert(message.id);
                                    m[message.key] = p;
                                }
                            }
                            else
                            {
                                if (!is_again(it, message))
                                {
                                    if (is_collision(it, message))
                                    {
                                        message.new_value = collision;
                                    }
                                    add(m, message);
                                    string str = mes2str(message);
                                    send2all(bufs, str, i);
                                }
                            }
                        }
                    }
                    bufs[i].second -= delim - 1;
                    memmove(bufs[i].first, bufs[i].first + delim + 1, 
                                sizeof(char) * (delim + 1)); 
                } 

            }
            if (fd[i].revents & POLLOUT)
            {
                if (bufs[i].second > 0)
                {
                    int tmp = write(fd[i].fd, bufs[i].first, bufs[i].second);
                    bufs[i].second -= tmp;
                    memmove(bufs[i].first, bufs[i].first + tmp, 
                            bufs[i].second * sizeof(char));
                }
            }
        }
        if (fd[1].revents && POLLIN)
        {
            int len = 
                read(STDIN, bufs[1].first + bufs[2].second, BUF_SIZE - bufs[2].second);
            bufs[1].second += len;
            if (int delim = check_ready(bufs[1]) != -1){
                if (check_correct(bufs[1], delim))
                {
                    mes message = parse(bufs[1], delim);
                    message.id += id;
                    id++;
                    add(m, message);
                    string str = mes2str(message);
                    send2all(bufs, str, 1);
                }
                bufs[1].second -= delim - 1;
                memmove(bufs[1].first, bufs[1].first + delim + 1, 
                                sizeof(char) * (delim + 1)); 
            }
            if (is_print(bufs[1]))
            {
                print(m);
            } 
        }
        if (fd[0].revents && POLLIN)
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
