#include "pipeline.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Expected 1 argument. Usage:\n  ./download ftp://...\n");
        return 0;
    }

    /**
     * 1. Parse input FTP URL
     */
    parse_url(argv[1]);

    /**
     * 2. Resolve hostname to server's IPv4 IP address and open protocol socket
     */
    ftp_open_protocol_socket();

    /**
     * 3. Login to the server (user + password)
     */
    ftp_login();

    /**
     * 4. Enter passive mode and open passive socket
     */
    ftp_open_passive_socket();

    /**
     * 5. Send retrieve command for file
     */
    send_retrieve();

    /**
     * 6. Download file
     */
    download_file();

    /**
     * 7. Close connection to server
     */
    return 0;
}
