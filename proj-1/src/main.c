#include "options.h"
#include "ll-setup.h"
#include "ll-interface.h"
#include "app-interface.h"
#include "app-core.h"
#include "signals.h"
#include "ll-frames.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* m = "O rato roeu a rolha da garrafa do rei da Russia";

static int main_transmitter() {
    set_signal_handlers();

    int fd = setup_link_layer(device);

    test_alarm();

    string msg = {m, strlen(m)};
    writeIframe(fd, msg, 1);

    reset_link_layer(fd);

    return 0;
}

static int main_receiver() {
    set_signal_handlers();

    int fd = setup_link_layer(device);

    test_alarm();

    frame f;

    readFrame(fd, &f);
    printf("Received %s\nisISframe: %d\n", f.data.s,
        (int)isIframe(f, 1));

    reset_link_layer(fd);

    return 0;
}

// Transmitter
int main(int argc, char** argv) {
    parse_args(argc, argv);

    if (my_role == RECEIVER) {
        return main_receiver();
    } else {
        return main_transmitter();
    }
}
