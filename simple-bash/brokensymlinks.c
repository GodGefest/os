#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

void write_(int fd, char * const s, size_t len)
{
    size_t printed = 0;
    while (printed < len)
    {
        ssize_t res = write(fd, s + printed, len - printed);
        if (res < 0)
            return;
        printed += res;
    }
}

void* my_alloc(size_t size)
{
    void *data = malloc(size);
    if (data == 0)
        _exit(EXIT_FAILURE);
    return data;
}

void find_broken_symlinks(char * const prefix)
{
    size_t len = strlen(prefix);
    const int max_length = len + NAME_MAX + 1;

    if (max_length > PATH_MAX)
    {
        static char *error_message = (char*) " is too long name.";
        write_(STDERR, prefix, strlen(prefix));
        write_(STDERR, error_message, strlen(error_message));
        return;
    }

    char *filename = (char*) my_alloc(max_length);
    memcpy(filename, prefix, len + 1);
    if (filename[len - 1] != '/')
    {
        filename[len] = '/';
        len++;
        filename[len] = 0;
    }
    DIR *dirp = opendir(prefix);
    if (dirp == NULL)
        return;
    struct dirent *file;
    while ((file = readdir(dirp)) != NULL)
    {
        if (strcmp(file->d_name, ".") != 0 && 
                strcmp(file->d_name, "..") != 0)
        {
            strcat(filename, file->d_name);
            if (file->d_type == DT_LNK && access(filename, F_OK) == -1)
            {
                size_t len1 = strlen(filename);
                filename[len1] = '\n';
                write_(1, filename, len1 + 1);
            }
            else if(file->d_type == DT_DIR)
                find_broken_symlinks(filename);
            filename[len] = 0;
        }
    }
    free(filename);
    closedir(dirp);
}

int main(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++)
        find_broken_symlinks(argv[i]);
    return 0;
}
