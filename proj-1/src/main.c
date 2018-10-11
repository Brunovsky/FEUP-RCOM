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
/*
// Transmitter
int main(int argc, char** argv) {
    parse_args(argc, argv);

    set_signal_handlers();

    int fd = setup_link_layer(device);

    test_alarm();

    writeIframe(fd, "Ate logo caralho", 0);

    reset_link_layer(fd);

    return 0;
}

*/
// Receiver
int main(int argc, char** argv) {
    parse_args(argc, argv);

    int fd = setup_link_layer(device);

    frame f;
    readFrame(fd, &f);

    printf("Received %s\nisIframe: %d\n", f.data, (int)isIframe(f, 0));

    reset_link_layer(fd);

    return 0;
}
