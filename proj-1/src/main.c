#include "options.h"
#include "ll-setup.h"
#include "ll-interface.h"
#include "app-layer.h"
#include "signals.h"
#include "fileio.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

char* m0 = "/home/malheiro/putas/ganso.png";
char* m1 = "O rato roeu a rolha da garrafa do rei da Russia";
char* m2 = "Putas tusas gay pelo Dux e pelo Marcelo";
char* m3 = "Top 10 Anime Messages -- and other stupid things";
char* m4 = "O Bagaco foi rapado e o Malheiro tambem";

static int main_transmitter() {
    set_signal_handlers();
    int fd = setup_link_layer(device);
    test_alarm();


    send_file(fd, "/tmp/lepenguin.jpg");


    sleep(1);
    reset_link_layer(fd);
    return 0;
}

static int main_receiver() {
    set_signal_handlers();
    int fd = setup_link_layer(device);
    test_alarm();


    receive_file(fd);


    sleep(1);
    reset_link_layer(fd);
    return 0;
}

int main(int argc, char** argv) {
    parse_args(argc, argv);

    if (my_role == RECEIVER) {
        return main_receiver();
    } else {
        return main_transmitter();
    }
}
