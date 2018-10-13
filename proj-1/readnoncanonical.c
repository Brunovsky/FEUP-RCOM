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
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv) {
    struct termios oldtio, newtio;

    if (argc < 2) {
      exit(1);
    }

    /*
      Open serial port device for reading and writing and not as controlling tty
      because we don't want to get killed if linenoise sends CTRL-C.
    */

    int fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) {
      perror(argv[1]);
      exit(1);
    }

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;   /* blocking read until 0 chars received */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    fflush(NULL);

    unsigned i = 0;
    char buf[16384];
    ssize_t res = 0;

    while (STOP == FALSE) {
      res = read(fd, buf + i, 1);
      if (res > 0) {
          printf("Read  %c  %d\n", buf[i], (int)buf[i]);
          if (buf[i++] == '\0') {
            STOP = TRUE;
          }
      } else if (res < 0) {
            perror("res < 0 --");
        }
    }

    printf(":%s:%d\n", buf, i);

    fflush(NULL);
    sleep(2);

    res = write(fd, buf, strlen(buf) + 1);
    printf("%ld bytes written\nFinished\n", res);
    sleep(1);

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
