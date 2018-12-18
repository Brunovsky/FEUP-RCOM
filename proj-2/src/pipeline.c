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
 * FTP Protocol Socket (User-Protocol Interpreter)
 */
static FILE* protocolstream = NULL;
static int protocolfd = 0;

/**
 * FTP Passive Socket (Data Transfer)
 */
static FILE* passivestream = NULL;
static int passivefd = 0;

/**
 * Input FTP URL
 */
static url_t url;

static void close_protocol_socket();

static void close_passive_socket();

static void free_url();

/**
 * Send an FTP command to the protocol socket.
 */
static int send_ftp_command(const char* command) {
    ssize_t count = 0, s = 0;
    ssize_t len = strlen(command);

    ftpcommand(command);

    do {
        count += s = send(protocolfd, command + count, len - count, 0);
        if (s <= 0) fail("Failed to write anything to protocol socket");
    } while (count < len);

    return 0;
}

/**
 * Read and discard reply lines from the FTP protocol socket stream until reading a
 * line that starts with an FTP code not followed by a dash.
 */
static int recv_ftp_reply(char* code, char** line) {
    char* reply = NULL;

    ssize_t read_size = 0;

    do {
        free(reply);
        
        reply = NULL;
        size_t n = 0;
        
        read_size = getline(&reply, &n, protocolstream); // keeps \r\n
        if (read_size < 1) fail("Failed to read reply from protocol socket stream");

        ftpreply(reply);
    } while ('1' > reply[0] || reply[0] > '5' || read_size < 4 || reply[3] == '-');

    strncpy(code, reply, 3); code[3] = '\0';
    line != NULL ? *line = reply : free(reply);

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
 * For URL parsing we use a weak regular expression. It accepts all valid (ftp) URLs
 * with non-empty filenames, although it also passes a whole bunch of invalid ones too.
 * It gets the job done quickly. Note: it does not accept a port after the host.
 */
int parse_url(char* urlstr) {
    regex_t regex;
    // capturers:     1       23        4 5             6         78           9
    //                 ftp ://[user     [:pass]      @] host     /path/  to/   filename
    regcomp(&regex, "^(ftp)://(([^:@/]+)(:([^:@/]+))?@)?([^:@/]+)/(([^:@/]+/)*)([^:@/]+)$",
        REG_EXTENDED | REG_ICASE | REG_NEWLINE);

    regmatch_t pmatch[10];

    // Regex-parse the input url
    int s = regexec(&regex, urlstr, 10, pmatch, 0);
    regfree(&regex);
    if (s != 0) fail("Invalid URL (failed quick regex parse)");

    // Take captures
    url = (url_t){
        .protocol = regexcap(urlstr, pmatch[1]),
        .username = regexcap(urlstr, pmatch[3]),
        .password = regexcap(urlstr, pmatch[5]),
        .hostname = regexcap(urlstr, pmatch[6]),
        .pathname = regexcap(urlstr, pmatch[7]),
        .filename = regexcap(urlstr, pmatch[9]),
        .port = 21
    };

    atexit(free_url);

    progress("1. FTP URL: ftp://%s/%s%s", url.hostname, url.pathname, url.filename);

    // Pick default username
    if (url.username == NULL) {
        url.username = strdup("anonymous"); // *** DEFAULT USER
        progress(" Defaulting username to %s", url.username);
    }

    // Pick default password
    if (url.password == NULL) {
        url.password = strdup("rcom-lab2"); // *** DEFAULT PASS
        progress(" Defaulting password to %s", url.password);
    }

    // Extra: warning for the common test case where host is ftp.up.pt
    if (strcmp(url.username, "anonymous") && !strcmp(url.hostname, "ftp.up.pt")) {
        printf("\n **** Warning **** ftp.up.pt operates only in anonymous mode.\n\n");
    }

    progress(" url.protocol: %s", url.protocol);
    progress(" url.username: %s", url.username);
    progress(" url.password: %s", url.password);
    progress(" url.hostname: %s", url.hostname);
    progress(" url.pathname: %s", url.pathname);
    progress(" url.filename: %s", url.filename);
    progress(" url.port: %d", url.port);

    return 0;
}

/**
 * 2. Resolve hostname to server's IPv4 address and setup protocol socket
 */
int ftp_open_protocol_socket() {
    char code[4];

    progress("2. Resolve hostname %s and setup protocol socket", url.hostname);

    // 2.1. resolve
    struct hostent* host = gethostbyname(url.hostname);
    if (host == NULL) fail("Could not resolve hostname %s", url.hostname);

    progress(" 2.1. Resolved hostname %s successfully", url.hostname);
    progress("  host->h_name: %s", host->h_name);
    progress("  host->h_addrtype: %d [IPv4=%d]", host->h_addrtype, AF_INET);

    // 2.2. extract IPv4 address
    if (host->h_addrtype != AF_INET) fail("Expected IPv4 address");

    char* protocolip = inet_ntoa(*(struct in_addr*)host->h_addr);

    progress(" 2.2. Protocol IP Address: %s", protocolip);
    progress("      Establishing protocol connection with FTP server");

    // 2.3. socket()
    protocolfd = socket(AF_INET, SOCK_STREAM, 0);
    if (protocolfd == -1) fail("Failed to open socket for protocol connection");

    protocolstream = fdopen(protocolfd, "r");
    atexit(close_protocol_socket);

    progress(" 2.3. Opened protocol socket for FTP's protocol connection");

    // 2.4. connect()
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET; // IPv4 address
    in_addr.sin_port = htons(url.port); // host byte order -> network byte order
    in_addr.sin_addr.s_addr = inet_addr(protocolip); // to network format

    int s = connect(protocolfd, (struct sockaddr*)&in_addr, sizeof(in_addr));
    if (s != 0) fail("Failed to connect() protocol socket");

    progress(" 2.4. Connected protocol socket to %s:%d", protocolip, 21);

    // 2.5. recv() 220 === Service ready for new user
    recv_ftp_reply(code, NULL);
    if (strcmp(code, "220") != 0) unexpected("Expected reply 220, got %s", code);

    progress(" 2.5. Successfully established protocol socket connection");

    return 0;
}

/**
 * 3. Login in the protocol.
 * Simply a USER command followed by a PASS command.
 * Must be careful to avoid multiple 220 from the previous connect()
 */
int ftp_login() {
    char code[4];

    progress("3. Send USER and PASS login commands to FTP server");

    // 3.1. send() USER username
    //      recv() 331 === User name okay, send password
    char* user_command = malloc((10 + strlen(url.username)) * sizeof(char));
    sprintf(user_command, "USER %s\r\n", url.username);

    send_ftp_command(user_command);
    free(user_command);

    recv_ftp_reply(code, NULL);
    if (strcmp(code, "331") != 0) unexpected("Expected reply 331, got %s", code);

    progress(" 3.1. Server confirmed user %s", url.username);

    // 3.2. send() PASS password
    //      recv() 230 === Login successful, proceed
    char* pass_command = malloc((10 + strlen(url.password)) * sizeof(char));
    sprintf(pass_command, "PASS %s\r\n", url.password);

    send_ftp_command(pass_command);
    free(pass_command);

    recv_ftp_reply(code, NULL);
    if (strcmp(code, "230") != 0) unexpected("Expected reply 230, got %s", code);

    progress(" 3.2. Server acknowledges login, proceeding");

    return 0;
}

/**
 * 4. Enter passive mode
 */
int ftp_open_passive_socket() {
    char code[4];

    progress("4. Establish passive connection with FTP server");

    // 4.1. send() PASV
    //      recv() 227 Entering Passive Mode (IP3.IP2.IP1.IP0,Port1,Port0)
    send_ftp_command("PASV \r\n");

    char* line;
    recv_ftp_reply(code, &line);
    if (strcmp(code, "227") != 0) unexpected("Expected reply 227, got %d", code);

    regex_t regex;
    regcomp(&regex, "([0-9]+, ?[0-9]+, ?[0-9]+, ?[0-9]+, ?[0-9]+, ?[0-9]+)",
        REG_EXTENDED | REG_ICASE);

    regmatch_t pmatch[2];

    int s = regexec(&regex, line, 2, pmatch, 0);
    regfree(&regex);
    if (s != 0) fail("Unexpected Passive Mode response format: %s", line);
    
    char* substr = regexcap(line, pmatch[1]);
    free(line);

    progress(" 4.1. Entered Passive Mode: (%s)", substr);

    // 4.2. extract passive ip from response line
    int ip3, ip2, ip1, ip0, port1, port0;
    sscanf(substr, "%d, %d, %d, %d, %d, %d", &ip3, &ip2, &ip1, &ip0, &port1, &port0);
    free(substr);

    char passiveip[20] = {0};
    sprintf(passiveip, "%d.%d.%d.%d", ip3, ip2, ip1, ip0);
    int port = port1 * 256 + port0;

    progress(" 4.2. Passive IP Address: %s:%d", passiveip, port);

    // 4.3. socket()
    passivefd = socket(AF_INET, SOCK_STREAM, 0);
    if (passivefd == -1) fail("Failed to open socket for passive connection");

    passivestream = fdopen(passivefd, "r+");
    atexit(close_passive_socket);

    progress(" 4.3. Opened passive socket for FTP's passive connection");

    // 4.4. connect()
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET; // IPv4 address
    in_addr.sin_port = htons(port); // host bytes -> network bytes
    in_addr.sin_addr.s_addr = inet_addr(passiveip); // to network format

    s = connect(passivefd, (struct sockaddr*)&in_addr, sizeof(in_addr));
    if (s != 0) fail("Failed to connect() passive socket");

    progress(" 4.4. Connected passive socket to %s:%d", passiveip, port);
    progress(" 4.5. Successfully established passive socket connection");

    return 0;
}

/**
 * 5. Send Retrieve command to FTP server
 * This command instructs the server to send the filename through the passive connection.
 */
int send_retrieve() {
    char code[4];

    progress("5. Retrieve file from Server (retrieve command)");

    // 5.1. send() RETR filepath
    //      recv() 150 File status okay, opening data transfer now
    size_t len = strlen(url.pathname) + strlen(url.filename);
    char* retr_command = malloc((10 + len) * sizeof(char));
    sprintf(retr_command, "RETR %s%s\r\n", url.pathname, url.filename);

    send_ftp_command(retr_command);
    free(retr_command);

    recv_ftp_reply(code, NULL);
    if (strcmp(code, "150") != 0) unexpected("Expected reply 150, got %s", code);

    progress(" 5.1. Confirmed, server retrieved %s%s", url.pathname, url.filename);

    return 0;
}

/**
 * 6. Download file retrieved.
 * It is being sent through TCP to port 20, we just read the socket
 * and forward to the output file stream until the transmission is over.
 */
int download_file() {
    char code[4];

    progress("6. Download file %s into current directory", url.filename);

    // 6.1. Open output file in current directory
    FILE* out = fopen(url.filename, "w"); // streams are love, streams are life
    if (out == NULL) fail("Failed to open output file in current directory");

    progress(" 6.1. Opened output file successfully");

    // 6.2. Canonical reading loop on passivefd, read the whole thing
    char buffer[1024]; // TCP caps at 1500B/p practically
    ssize_t read_size = 0;

    progress("      Reading...");

    while ((read_size = read(passivefd, buffer, sizeof(buffer))) > 0) {
        size_t write_size = fwrite(buffer, read_size, 1, out);
        if (write_size != 1) fail("Failed to write to output file");
    }

    if (read_size < 0) fail("Failed to read file on passive socket");

    fclose(out);

    progress(" 6.2. Done.");

    // 6.3. recv() 226 === Closing data connection, transfer complete
    recv_ftp_reply(code, NULL);
    if (strcmp(code, "226") != 0) unexpected("Expected reply 226, got %d", code);

    return 0;
}

/**
 * 7. Close the connection to the server.
 */
static void close_protocol_socket() {
    send_ftp_command("QUIT \r\n");
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
