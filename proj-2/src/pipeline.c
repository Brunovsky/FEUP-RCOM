#include "pipeline.h"
#include "debug.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * FTP Protocol Socket
 */
static FILE* protocolstream = NULL;
static int protocolfd = 0;

/**
 * FTP Passive Socket
 */
static FILE* passivestream = NULL;
static int passivefd = 0;

static url_t url;

static void close_protocol_socket();

static void close_passive_socket();

static void free_url();

/**
 * Send an FTP command to a socket (just writes, really).
 */
static int send_ftp_command(int socketfd, const char* command) {
    ssize_t count = 0, s = 0;
    ssize_t len = strlen(command);

    ftpcommand(command);

    do {
        count += s = send(socketfd, command + count, len - count, 0);
        if (s <= 0) fail("Failed to write anything to socket");
    } while (count < len);

    return 0;
}

/**
 * Read and discard reply lines from an FTP socket stream until reading a
 * line that starts with an FTP code (weak check).
 */
static int recv_ftp_reply(FILE* socketsm, char* code, char** line) {
    char* reply = NULL;
    size_t n = 0;

    char first_char;
    ssize_t read_count = 0;

    do {
        free(reply);
        
        reply = NULL;
        n = 0;
        
        read_count = getline(&reply, &n, socketsm); // keeps \r\n
        if (read_count < 1) fail("Failed to read reply from socket stream");

        ftpreply(reply);
        first_char = reply[0];
    } while ('1' > first_char || first_char > '5' || read_count < 5);

    strncpy(code, reply, 3); code[3] = '\0';
    if (line != NULL) {
        *line = reply;
    } else {
        free(reply);
    }
    return 0;
}

/**
 * Read and discard reply lines from an FTP socket stream until receiving a
 * line that starts with an FTP code (weak check) which is not listed in excl.
 */
static int recv_ftp_replies_until(FILE* socketsm, char* code, char** line, char* excl) {
    ftpsocketio("@@%s", excl);

    char codebuf[4] = {0};
    char* linebuf = NULL;

    do {
        free(linebuf);
        recv_ftp_reply(socketsm, codebuf, &linebuf);
    } while (strstr(excl, codebuf) != NULL);

    strncpy(code, codebuf, 4);
    if (line != NULL) {
        *line = linebuf;
    } else {
        free(linebuf);
    }
    return 0;
}

/**
 * Extract substring matched by a capture group from the source string,
 * returning NULL if the capture group didn't match.
 */
static char* regexcap(char* source, regmatch_t match) {
    ssize_t start = match.rm_so;
    ssize_t end = match.rm_eo;
    if (start == -1) return NULL;

    size_t len = end - start;
    
    char* buffer = malloc((len + 1) * sizeof(char));
    strncpy(buffer, source + start, len);
    buffer[len] = '\0';

    return buffer;
}

/**
 * 1. Parse program's input, the FTP URL
 * For the URL parsing we use a weak regular expression. It passes all valid (ftp) URLs
 * with non-empty filenames, although it also passes a whole bunch of invalid ones too.
 * It gets the job done quickly.
 */
url_t parse_url(char* urlstr) {
    regex_t regex;
    // captures:      1       23       4 5            6         78           9
    //                 ftp ://[user    [:pass]     @]?host   /path/to/   filename
    regcomp(&regex, "^(ftp)://(([^:/]+)(:([^@/]+))?@)?([^:@/]+)/(([^:@/]+/)*)([^:@/]+)$",
        REG_EXTENDED | REG_ICASE | REG_NEWLINE);

    regmatch_t pmatch[10];

    int s = regexec(&regex, urlstr, 10, pmatch, 0);
    regfree(&regex);
    if (s != 0) fail("Invalid URL (failed quick regex parse)");

    url = (url_t){
        .protocol = regexcap(urlstr, pmatch[1]),
        .username = regexcap(urlstr, pmatch[3]),
        .password = regexcap(urlstr, pmatch[5]),
        .hostname = regexcap(urlstr, pmatch[6]),
        .pathname = regexcap(urlstr, pmatch[7]),
        .filename = regexcap(urlstr, pmatch[9])
    };

    progress("1. Valid URL: ftp://%s/%s%s", url.hostname, url.pathname, url.filename);

    if (url.username == NULL) {
        url.username = strdup("anonymous"); // *** DEFAULT USER
        progress("Defaulting username to %s", url.username);
    }
    if (url.password == NULL) {
        url.password = strdup("up201606517@fe.up.pt"); // *** DEFAULT PASS
        progress("Defaulting password to %s", url.password);
    }
    if (strcmp(url.username, "anonymous") && !strcmp(url.hostname, "ftp.up.pt")) {
        printf("\n***** Warning: ftp.up.pt operates only in anonymous mode.\n\n");
    }

    progress(" url.protocol: %s", url.protocol);
    progress(" url.username: %s", url.username);
    progress(" url.password: %s", url.password);
    progress(" url.hostname: %s", url.hostname);
    progress(" url.pathname: %s", url.pathname);
    progress(" url.filename: %s", url.filename);

    atexit(free_url);
    return url;
}

/**
 * 2. Resolve hostname to server's IPv4 IP address and open protocol socket
 */
int ftp_open_protocol_socket(const char* hostname) {
    char code[4];

    progress("2. Resolve hostname %s and open protocol socket", hostname);

    struct hostent* host = gethostbyname(hostname);
    if (host == NULL) fail("Could not resolve hostname %s", hostname);

    progress(" 2.1. Resolved hostname %s successfully", hostname);
    progress("  host->h_name: %s", host->h_name);
    progress("  host->h_addrtype: %d [AF_INET=%d]", host->h_addrtype, AF_INET);
    progress("  host->h_length: %d", host->h_length);

    if (host->h_addrtype != AF_INET) fail("Expected IPv4 address");

    char* protocolip = inet_ntoa(*(struct in_addr*)host->h_addr);

    progress(" 2.2. Protocol IP Address: %s", protocolip);

    progress(" 2.3. Establishing protocol connection with FTP server");

    // 3.1. socket()
    protocolfd = socket(AF_INET, SOCK_STREAM, 0);
    if (protocolfd == -1) fail("Failed to open socket for protocol connection");

    protocolstream = fdopen(protocolfd, "r");
    atexit(close_protocol_socket);

    progress(" 2.4. Opened protocol socket for FTP's protocol connection");

    // 3.2. connect()
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(21);
    in_addr.sin_addr.s_addr = inet_addr(protocolip);

    int s = connect(protocolfd, (struct sockaddr*)&in_addr, sizeof(in_addr));
    if (s != 0) fail("Failed to connect() protocol socket");

    progress(" 2.5. Connected protocol socket to %s:%d", protocolip, 21);

    // 3.3. recv() 220
    recv_ftp_reply(protocolstream, code, NULL);

    if (strcmp(code, "220") != 0) fail("Expected message code 220");

    progress(" 2.6. Successful protocol socket connection established");

    return 0;
}

/**
 * 3. Login
 */
int ftp_login(const char* username, const char* password) {
    char code[4];

    progress("3. Send login commands to FTP server");

    // 3.1. send() USER username
    char* user_command = malloc((10 + strlen(username)) * sizeof(char));
    sprintf(user_command, "USER %s\r\n", username);

    send_ftp_command(protocolfd, user_command);
    free(user_command);

    progress(" 3.1. Login: Sent command USER %s", username);

    // 3.2. recv() 331
    recv_ftp_replies_until(protocolstream, code, NULL, "220");

    if (strcmp(code, "331") != 0) fail("Expected reply 331, got %s", code);

    progress(" 3.2. Confirmed, server requesting password");

    // 3.3. send() PASS password
    char* pass_command = malloc((15 + strlen(password)) * sizeof(char));
    sprintf(pass_command, "PASS %s\r\n", password);

    send_ftp_command(protocolfd, pass_command);
    free(pass_command);

    progress(" 3.3. Login: Sent command PASS %s", password);

    // 3.4. recv() 230
    recv_ftp_replies_until(protocolstream, code, NULL, "331");

    if (strcmp(code, "230") != 0) fail("Expected reply 230, got %s", code);

    progress(" 3.4. Confirmed, server acknowledges login");

    return 0;
}

/**
 * 4. Enter passive mode
 */
int ftp_open_passive_socket() {
    char code[4];

    progress("4. Establish passive connection with FTP server");

    // 4.1. send() PASV
    send_ftp_command(protocolfd, "PASV\r\n");

    progress(" 4.1. Sent command PASV");

    // 4.2. recv() 227 Entering Passive Mode (IP3,IP2,IP1,IP0,Port1,Port0)
    char* line = NULL;
    recv_ftp_reply(protocolstream, code, &line);

    if (strcmp(code, "227") != 0) fail("Expected reply 227, got %d", code);

    regex_t regex;
    regcomp(&regex, "([0-9]+, ?[0-9]+, ?[0-9]+, ?[0-9]+, ?[0-9]+, ?[0-9]+)",
        REG_EXTENDED | REG_ICASE);

    regmatch_t pmatch[2];

    int s = regexec(&regex, line, 2, pmatch, 0);
    regfree(&regex);
    if (s != 0) fail("Unexpected Passive Mode response format: %s", line);
    
    char* substr = regexcap(line, pmatch[1]);
    free(line);

    progress(" 4.2. Passive Mode: %s", substr);

    // 4.3. extract passive ip from response line
    int ip3, ip2, ip1, ip0, port1, port0;
    sscanf(substr, "%d, %d, %d, %d, %d, %d", &ip3, &ip2, &ip1, &ip0, &port1, &port0);
    free(substr);

    char passiveip[20] = {0};
    sprintf(passiveip, "%d.%d.%d.%d", ip3, ip2, ip1, ip0);
    int port = port1 * 256 + port0;

    progress(" 4.3. Passive IP Address: %s:%d", passiveip, port);

    // 4.4. socket()
    passivefd = socket(AF_INET, SOCK_STREAM, 0);
    if (passivefd == -1) fail("Failed to open socket for passive connection");

    passivestream = fdopen(passivefd, "r+");
    atexit(close_passive_socket);

    progress(" 4.4. Opened passive socket for FTP's passive connection");

    // 4.5. connect()
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(port);
    in_addr.sin_addr.s_addr = inet_addr(passiveip);

    s = connect(passivefd, (struct sockaddr*)&in_addr, sizeof(in_addr));
    if (s != 0) fail("Failed to connect() passive socket");

    progress(" 4.5. Connected passive socket to %s:%d", passiveip, port);
    progress("      Successful passive socket connection established");

    return 0;
}

/**
 * 5. Send Retrieve command
 */
int send_retrieve(const char* pathname, const char* filename) {
    char code[4];

    progress("5. Send Retrieve command");

    // 5.1. send() RETR filepath/filename
    char* retr_command = malloc((10 + strlen(pathname) + strlen(filename)) * sizeof(char));
    sprintf(retr_command, "RETR %s%s\r\n", pathname, filename);

    send_ftp_command(protocolfd, retr_command);
    free(retr_command);

    progress(" 5.1. Retrieve: Sent command RETR %s%s", pathname, filename);

    // 5.2. recv() 150
    recv_ftp_reply(protocolstream, code, NULL);

    if (strcmp(code, "150") != 0) fail("Expected reply 150, got %s", code);

    progress(" 5.2. Confirmed, server retrieved %s%s", pathname, filename);

    return 0;
}

/**
 * 6. Download file.
 */
int download_file(const char* filename) {
    progress("6. Download file %s into current directory", filename);

    // 6.1. Open output file in current directory
    FILE* out = fopen(filename, "w"); // because I don't feel like choosing permissions
    if (out == NULL) fail("Failed to open output file in current directory");

    progress(" 6.1. Opened output file successfully");

    // 6.2. Canonical reading loop on passivefd, read the whole thing
    char buffer[2048];
    ssize_t read_count = 0;

    while ((read_count = read(passivefd, buffer, sizeof(buffer))) > 0) {
        size_t write_count = fwrite(buffer, read_count, 1, out);
        if (write_count != 1) fail("Failed to write to output file");
    }

    if (read_count < 0) fail("Failed to read file on passive socket");

    fclose(out);

    return 0;
}

/**
 * 7. Close the connection to the server.
 */
static void close_protocol_socket() {
    send_ftp_command(protocolfd, "QUIT\r\n");
    fclose(protocolstream);
}

static void close_passive_socket() {
    fclose(passivestream);
}

static void free_url() {
    free(url.protocol);
    free(url.username);
    free(url.password);
    free(url.hostname);
    free(url.pathname);
    free(url.filename);
}
