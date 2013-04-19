#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

size_t _strlen(char * const s)
{
    size_t len;
    for (len = 0; s[len] != 0; len++)
    {}
    return len;
}

int strings_differ(char * const s1, char * const s2)
{
    size_t l1 = _strlen(s1);
    size_t l2 = _strlen(s2);
    if (l1 != l2)
    {
        return 1;
    }
    int i;
    for (i = 0; i < l1; i++)
    {
        if (s1[i] != s2[i])
        {
            return 1;
        }
    }
    return 0;
}

void _strcat(char *dest, char * const src)
{
    size_t l1 = _strlen(dest);
    size_t l2 = _strlen(src);
    memmove(dest + l1, src, sizeof(char) * (l2 + 1));
}

void _write(int fd, char * const s, size_t len)
{
    size_t printed = 0;
    while (printed < len)
    {
        ssize_t res = write(fd, s + printed, len - printed);
        if (res < 0)
        {
            return;
        }
        printed += res;
    }
}

void find_broken_symlinks(char * const prefix)
{
    size_t len = _strlen(prefix);
    char *filename = malloc(len + 260);
    memcpy(filename, prefix, sizeof(char) * (len + 1));
    if (filename[len - 1] != '/')
    {
        filename[len] = '/';
        len++;
        filename[len] = 0;
    }
    DIR *dirp = opendir(prefix);
    if (dirp == NULL)
    {
        return;
    }
    struct dirent *file;
    while ((file = readdir(dirp)) != NULL)
    {
        if (strings_differ(file->d_name, ".") && 
                strings_differ(file->d_name, "..") &&
                file->d_name[0] != '.')
        {
            _strcat(filename, file->d_name);
            if ((file->d_type == DT_LNK) && (access(filename, F_OK) == -1))
            {
                size_t len1 = _strlen(filename);
                filename[len1] = '\n';
                _write(1, filename, len1 + 1);
            }
            else if(file->d_type == DT_DIR)
            {
                find_broken_symlinks(filename);
            }
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
    {
        find_broken_symlinks(argv[i]);
    }
    return 0;
}
