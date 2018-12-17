#include "pipeline.h"

#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    /**
     * 1. Parse input FTP URL
     */
    url_t url = parse_url(argv[1]);

    /**
     * 2. Resolve hostname to server's IPv4 IP address and open protocol socket
     */
    ftp_open_protocol_socket(url.hostname);

    /**
     * 3. Login to the server (user + password)
     */
    ftp_login(url.username, url.password);

    /**
     * 4. Enter passive mode and open passive socket
     */
    ftp_open_passive_socket();

    /**
     * 5. Send retrieve command for file
     */
    send_retrieve(url.pathname, url.filename);

    /**
     * 6. Download file
     */
    download_file(url.filename);

    /**
     * 7. Close connection to server
     */
    exit(EXIT_SUCCESS);
}
