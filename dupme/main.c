#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int k;
    k = atoi(argv[1]);
    printf("k = %d", k);
    return 0;
}
