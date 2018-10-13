#include "options.h"
#include "ll-setup.h"
#include "ll-interface.h"
#include "app-layer.h"
#include "signals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

char* m1 = "O rato roeu a rolha da garrafa do rei da Russia";
char* m2 = "Putas tusas gay pelo Dux e pelo Marcelo";
char* m3 = "Top 10 Anime Messages -- and other stupid things";
char* m4 = "O Bagaco foi rapado e o Malheiro tambem";

static int main_transmitter() {
    set_signal_handlers();
    int fd = setup_link_layer(device);
    test_alarm();


    string message1 = {m1, strlen(m1)};
    string message2 = {m2, strlen(m2)};
    string message3 = {m3, strlen(m3)};
    string message4 = {m4, strlen(m4)};

    llopen(fd);

    send_start_packet(fd, 73, "O Malheiro comeu o Dux");

    send_data_packet(fd, message1);
    send_data_packet(fd, message2);
    send_data_packet(fd, message3);
    send_data_packet(fd, message4);

    send_end_packet(fd, 73, "O Malheiro comeu o Dux");

    llclose(fd);


    sleep(1);
    reset_link_layer(fd);
    return 0;
}

static int main_receiver() {
    set_signal_handlers();
    int fd = setup_link_layer(device);
    test_alarm();


    llopen(fd);

    data_packet dp, dp1, dp2, dp3, dp4;
    control_packet cp, cp1, cp2;

    receive_packet(fd, &dp, &cp1);
    receive_packet(fd, &dp1, &cp);
    receive_packet(fd, &dp2, &cp);
    receive_packet(fd, &dp3, &cp);
    receive_packet(fd, &dp4, &cp);
    receive_packet(fd, &dp, &cp2);

    llclose(fd);


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
