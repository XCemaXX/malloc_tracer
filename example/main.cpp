#include <array>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

bool run = true;

void signal_handler(int signum) {
    printf("Caught signal %d\n", signum);
    run = false;
}

struct my_struct120 {
    int                     v = 0x120;
    std::array<int64_t, 10> ints{0xaaaa, 0, 0xaaaa, 0, 0xaaaa, 0xffff};
    std::string             s = "abcdefgh";
};

struct my_struct40 {
    int         v = 0x50;
    std::string s = "1234567";
};

struct my_struct19 {
    std::array<char, 19> v{0x19, '1', '9', 'r', 't', 'y', 'u', 'i', '1', '9', 0x19};
};

struct my_struct23 {
    std::array<char, 23> v{0x23, 0x23, '2', '3', 'e', 'r', 't', 'y', 'u', 'i', '2', '3', 0x23, 0x23};
};
// template<int s> struct Wow;
// Wow<sizeof(my_struct23)> wow;

int main(int, char**) {
    auto a = new my_struct120();
    auto b = new my_struct40();

    void* ptr_c = NULL;
    posix_memalign(&ptr_c, 1024, sizeof(my_struct19)); // new my_struct19();
    auto c = new (ptr_c) my_struct19();
    delete a;
    auto arr = new my_struct23[5];
    auto d = new my_struct23();

    signal(SIGINT, signal_handler);
    std::cout << "Hello, from hello_world!\n";
    unsigned char i = 255;

    char* big100Mb = (char*)calloc(100 * 1024 * 1024, 1);
    char* big10Mb = (char*)calloc(10 * 1024 * 1024, 1);
    sprintf(big100Mb, "big100Mb. A lot of data");
    sprintf(big10Mb, "big10Mb. A lot of data");

    while (run) {
        ++i;
        std::cout << "Pid " << ::getpid() << " " << (int)i;
        printf(" big100Mb is %p; ", big100Mb);
        printf("addr10Mb is %p\n", big10Mb);
        sleep(1);
    }
    delete b;
    free(ptr_c); // delete c;
    delete d;
    delete[] arr;
    free(big100Mb);
    free(big10Mb);

    auto z = malloc(20);
    z = realloc(z, 40);
    z = realloc(z, 80);
    free(z);
    return EXIT_SUCCESS;
}
