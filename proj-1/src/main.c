#include "options.h"
#include "ll-setup.h"
#include "ll-interface.h"
#include "app-interface.h"
#include "app-core.h"
#include "signals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Transmitter
int main(int argc, char** argv) {
    parse_args(argc, argv);

    printf("Setup link layer\n");
    int fd = setup_link_layer(device);

    int s = 0;

    test_alarm();

    s = llopen(fd, TRANSMITTER);

    if (s != 0) {
        printf("Failed llopen %d\n", s);
    }

    char* message = "Ate logo lol";
    string str = {message, strlen(message)};

    s = llwrite(fd, str);

    if (s != 0) {
        printf("Failed llwrite %d\n", s);
    }

    s = llclose(fd, TRANSMITTER);

    if (s != 0) {
        printf("Failed llclose %d\n", s);
    }

    reset_link_layer(fd);

    return 0;
}

/*
// Receiver
int main(int argc, char** argv) {
    parse_args(argc, argv);

    int fd = setup_link_layer(device);

    int s = 0;

    s = llopen(fd, RECEIVER);

    if (s != 0) {
        printf("Failed llopen %d\n", s);
    }

    string buf;
    s = llread(fd, &buf);

    if (s != 0) {
        printf("Failed llwrite %d\n", s);
    } else {
        printf("Mensagem: %s\n", buf.s);
    }

    s = llclose(fd, RECEIVER);

    if (s != 0) {
        printf("Failed llclose %d\n", s);
    }

    reset_link_layer(fd);

    free(buf);

    return 0;
}
*/