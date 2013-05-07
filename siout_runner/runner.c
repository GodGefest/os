#include <unistd.h>

#define STDIN 0
#define STDOUT 1

int main(int argc, char** argv) {
    char* source;
    if (argc != 1)
    {
        return -1;
    }

    source = argv[1];

    return 0;
}
