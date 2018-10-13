/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BAUDRATE B38400
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;

int main(int argc, char** argv) {
    struct termios oldtio, newtio;

    if (argc < 2) {
        printf("Missing device argument");
        exit(1);
    }

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */

    int fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    printf("Opened tty\n");

    if ( tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(1);
    }

    printf("Read attributes\n");

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;   /* blocking read until 0 chars received */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(1);
    }

    printf("New termios structure set\n");

    char msg[] = "O rato roeu a rolha da garrafa do rei da Russia";

    ssize_t res = write(fd, msg, strlen(msg) + 1);

    fflush(NULL);

    printf("%ld bytes written\n", res);
    ssize_t old = res;

    /*
    O ciclo FOR e as instrucoes seguintes devem ser alterados de modo a respeitar
    o indicado no guiao
    */

    sleep(1);

    // Ler de volta

    STOP = FALSE;
    ssize_t i = 0;

    char buf[16384];

    while (STOP == FALSE) {
        res = read(fd, buf + i, 1);
        if (res > 0) {
            printf("Read  %c  %d\n", buf[i], (int)buf[i]);
            ++i;
            if (buf[i - 1] == '\0' && i >= old) {
              STOP = TRUE;
            }
            if (i >= 300) {
              STOP = TRUE;
            }
        } else if (res < 0) {
            perror("res < 0 --");
        }
    }
 
    printf(":%s:%ld\nFinished\n", buf, i);
    fflush(NULL);
    sleep(1);

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
