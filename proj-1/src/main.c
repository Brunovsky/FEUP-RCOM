#include "options.h"
#include "signals.h"
#include "fileio.h"
#include "ll-setup.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static void adjust_args() {
    if (packetsize > MAXIMUM_PACKET_SIZE) {
        printf("[MAIN] packetsize lowered from %lu to maximum size %lu\n",
            packetsize, MAXIMUM_PACKET_SIZE);
        packetsize = MAXIMUM_PACKET_SIZE;
    }
}

int main(int argc, char** argv) {
    parse_args(argc, argv);
    adjust_args();

    set_signal_handlers();
    
    test_alarm();
    
    int fd = setup_link_layer(device);

    if (my_role == TRANSMITTER) {
        send_file(fd, "/tmp/lepenguin.jpg");
    } else {
        receive_file(fd);
    }

    sleep(1);
    reset_link_layer(fd);
    return 0;
}
