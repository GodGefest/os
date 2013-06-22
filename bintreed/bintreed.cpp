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

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define OK 0
#define BAD_PATH 1
#define BAD_TREE 2
#define BAD_COMMAND 3

#define MAX_COUNT_OF_CONNECTIONS 5

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

struct node{
    string str;
    node *left;
    node *right;

    node() : str("()"), left(NULL), right(NULL) {}
    node(string str) : str(str), left(new node()), right(new node()) {}

    void remove() 
    {
        if (left != NULL)
        {
            delete left;
        }
        if (right != NULL)
        {
            delete right;
        }
    }

    ~node()
    {
        remove();
    }
};

size_t find_close_bracket(const string &str, int from)
{
    int state = 1;
    for(size_t i = from; i < str.size(); i++)
    {
        if(str[i] == '<') 
        {
            state++;
        }
        if(str[i] == '>')
        {
            state--;
            if (state == 0) 
            {
                return i;
            }
        }
    }
    return -1;
}

size_t find_open_bracket(const string &str, int from)
{
    for (size_t i = from; i < str.size(); i++)
    {
        if (str[i] == '<')
        {
            return i;
        }
    }
    return -1;
}

node *str2node(string str)
{
    if (str[0] == '(' && str[1] == ')')
    {
        return new node();
    }
    else
    {
        if (str[0] == '<')
        {
            //first open(close) bracket
            int fob, fcb;
            //second open(close) bracket
            int sob, scb;
            node *tree = new node();
            fob = 0;
            fcb = find_close_bracket(str, 1);
            if (fcb == -1)
            {
                return NULL;
            }
            tree->left = str2node(str.substr(fob + 1, fcb - fob - 1));
            sob = find_open_bracket(str, fcb);
            tree->str = str.substr(fcb + 1, sob - fcb - 1);
            scb = find_close_bracket(str, sob);
            tree->right = str2node(str.substr(sob + 1, scb - sob - 1));
            if (tree->left == NULL || tree->right == NULL)
            {
                delete tree;
                return NULL;
            }
            return tree;
        }
        else 
        {
            return NULL;
        }
    }
}

string node2str(node *tree)
{
    if (tree->str == "()")
    {
        return "()";
    }
    else
    {
        return '<' + node2str(tree->left) + '>' + tree->str + 
                '<' + node2str(tree->right) + '>';
    }
}

node *find_node(node *tree, string path)
{
    char c;
    for (size_t i = 0; i < path.size(); i++)
    {
        c = path[i];
        if (c == 'l')
        {
            if (tree->left != NULL)
            {
                tree = tree->left;
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            if (c == 'r')
            {
                if (tree->right != NULL)
                {
                    tree = tree->right;
                }
                else
                {
                    return NULL;
                }
            }
            else
            {
                if (c == 'h')
                {
                    return tree;
                }
                else
                {
                    return NULL;
                }
            }
        }
    }
    return NULL;
}

int add(node *tree, string path, string tree_desc)
{
    node *where = find_node(tree, path);
    node *what = str2node(tree_desc);
    if (where == NULL)
    {
        return BAD_PATH;
    }
    if (what == NULL)
    {
        return BAD_TREE;
    }
    delete where->left;
    delete where->right;
    where->str = what->str;
    where->left = what->left;
    where->right = what->right;
    return OK;
}

int del(node *tree, string path)
{
    return add(tree, path, "()");
}

string print(node *tree, string path)
{
    node *what = find_node(tree, path);
    if (what == NULL)
    {
        return "error: wrong path";
    }
    return "ok: " + node2str(what); 
}

const string bad_path_mes = "bad path\n";
const string bad_tree_mes = "bad tree\n";
const string ok_mes = "ok\n";

void print_res(int fd, int status)
{
    
    if (status == BAD_PATH)
    {
        my_write(fd, bad_path_mes.c_str(), bad_path_mes.size());
    }
    if (status == BAD_TREE)
    {
        my_write(fd, bad_tree_mes.c_str(), bad_tree_mes.size());
    }
    if (status == OK)
    {
        my_write(fd, ok_mes.c_str(), ok_mes.size());
    }
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

        int res_getaddrinfo = getaddrinfo(NULL, "8823", &hints, &result);
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
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        freeaddrinfo(result);
        
        if (listen(sfd, MAX_COUNT_OF_CONNECTIONS) == -1)
        {
            perror("Listen failed");
            exit(EXIT_FAILURE);
        }

        pollfd fd[MAX_COUNT_OF_CONNECTIONS + 1];
        fd[0].fd = sfd;
        fd[0].events = POLLIN;

        struct sockaddr address;
        socklen_t address_len = sizeof(address);

        node *root = new node();
        int clients = 1;
        int buffer_size = 4096;
        char *buffer = (char *) my_malloc(buffer_size);
        while (true)
        {
            my_poll(fd, clients);
            for (int i = 1; i < clients; i++)
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
                     int len = my_read(fd[i].fd, buffer, buffer_size);
                     string mes;
                     buffer[len - 1] = '\0';
                     string s(buffer);
                     size_t first_space = s.find(' ');
                     size_t second_space = s.find(' ', first_space + 1);
                     if (second_space == string::npos)
                     {
                        second_space = s.size();
                     }
                     string command = s.substr(0, first_space);
                     string path = 
                         s.substr(first_space + 1, second_space - first_space - 1);
                     if (command == "add") 
                     {
                         if (second_space != s.size())
                         {
                             string tree_desc = 
                                 s.substr(second_space + 1, s.size() - second_space - 1);
                             int add_ans = add(root, path, tree_desc);
                             print_res(fd[i].fd, add_ans);
                         }
                         else
                         {
                             print_res(fd[i].fd, BAD_PATH);
                         }
                     } 
                     else
                     {
                         if (command == "print")
                         {
                            string res = print(root, path);
                            my_write(fd[i].fd, res.c_str(), res.size());
                         }
                         else
                         {
                            if (command == "del")
                            {
                                int del_res = del(root, path);
                                print_res(fd[i].fd, del_res);
                            }
                            else
                            {
                                print_res(fd[i].fd, BAD_COMMAND);
                            }
                         }
                     }
                }
            }
            if (fd[0].revents && POLLIN)
            {
                int cfd = accept(sfd, &address, &address_len); 
                if (cfd == -1)
                {
                    perror("accept");
                }
                fd[clients].fd = cfd;
                fd[clients].events = POLLIN;
                clients++;
            }

        }

        close(sfd);
    }
    return 0;
}
