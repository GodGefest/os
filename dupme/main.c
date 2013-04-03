#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int k;
    if (argc < 2) {
        return 1;
    }
    k = atoi(argv[1]);
    if (k < 1) {
        return 1;
    }
    printf("k = %d\n", k);
    return 0;
}
