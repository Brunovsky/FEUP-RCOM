## FTP

### Commands

[RFC959 FTP Commands](https://www.w3.org/Protocols/rfc959/4_FileTransfer.html)

    USER username
        The argument field is a Telnet string identifying the user.
        The user identification is that which is required by the
        server for access to its file system.  This command will
        normally be the first command transmitted by the user after
        the control connections are made (some servers may require
        this).  Additional identification information in the form of
        a password and/or an account command may also be required by
        some servers.  Servers may allow a new USER command to be
        entered at any point in order to change the access control
        and/or accounting information.  This has the effect of
        flushing any user, password, and account information already
        supplied and beginning the login sequence again.  All
        transfer parameters are unchanged and any file transfer in
        progress is completed under the old access control
        parameters.

    PASS password
        The argument field is a Telnet string specifying the user's
        password.  This command must be immediately preceded by the
        user name command, and, for some sites, completes the user's
        identification for access control.  Since password
        information is quite sensitive, it is desirable in general
        to "mask" it or suppress typeout.  It appears that the
        server has no foolproof way to achieve this.  It is
        therefore the responsibility of the user-FTP process to hide
        the sensitive password information.

    PASV
        This command requests the server-DTP to "listen" on a data
        port (which is not its default data port) and to wait for a
        connection rather than initiate one upon receipt of a
        transfer command.  The response to this command includes the
        host and port address this server is listening on.

    TYPE
        The argument specifies the representation type as described
        in the Section on Data Representation and Storage.  Several
        types take a second parameter.  The first parameter is
        denoted by a single Telnet character, as is the second
        Format parameter for ASCII and EBCDIC; the second parameter
        for local byte is a decimal integer to indicate Bytesize.
        The parameters are separated by a <SP> (Space, ASCII code
        32).

        The following codes are assigned for type:

                     \    /
           A - ASCII |    | N - Non-print
                     |-><-| T - Telnet format effectors
           E - EBCDIC|    | C - Carriage Control (ASA)
                     /    \
           I - Image
           
           L <byte size> - Local byte Byte size


        The default representation type is ASCII Non-print.  If the
        Format parameter is changed, and later just the first
        argument is changed, Format then returns to the Non-print
        default.

    RETR path/to/file
        This command causes the server-DTP to transfer a copy of the
        file, specified in the pathname, to the server- or user-DTP
        at the other end of the data connection.  The status and
        contents of the file at the server site shall be unaffected.

    QUIT
        This command terminates a USER and if file transfer is not
        in progress, the server closes the control connection.  If
        file transfer is in progress, the connection will remain
        open for result response and the server will then close it.
        If the user-process is transferring files for several USERs
        but does not wish to close and then reopen connections for
        each, then the REIN command should be used instead of QUIT.

        An unexpected close on the control connection will cause the
        server to take the effective action of an abort (ABOR) and a
        logout (QUIT).

### Reply Codes

        110 Restart marker reply.
            In this case, the text is exact and not left to the
            particular implementation; it must read:
              MARK yyyy = mmmm
            Where yyyy is User-process data stream marker, and mmmm
            server's equivalent marker (note the spaces between markers
            and "=").
        120 Service ready in nnn minutes.
        125 Data connection already open; transfer starting.
    --> 150 File status okay; about to open data connection.
    --> 200 Command okay.
        202 Command not implemented, superfluous at this site.
        211 System status, or system help reply.
        212 Directory status.
        213 File status.
        214 Help message.
            On how to use the server or the meaning of a particular
            non-standard command.  This reply is useful only to the
            human user.
        215 NAME system type.
            Where NAME is an official system name from the list in the
            Assigned Numbers document.
    --> 220 Service ready for new user.
    --> 221 Service closing control connection.
            Logged out if appropriate.
        225 Data connection open; no transfer in progress.
    --> 226 Closing data connection.
            Requested file action successful (for example, file
            transfer or file abort).
    --> 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
    --> 230 User logged in, proceed.
        250 Requested file action okay, completed.
        257 "PATHNAME" created.

    --> 331 User name okay, need password.
        332 Need account for login.
        350 Requested file action pending further information.

        421 Service not available, closing control connection.
            This may be a reply to any command if the service knows it
            must shut down.
        425 Can't open data connection.
        426 Connection closed; transfer aborted.
        450 Requested file action not taken.
            File unavailable (e.g., file busy).
        451 Requested action aborted: local error in processing.
        452 Requested action not taken.
            Insufficient storage space in system.
        500 Syntax error, command unrecognized.
            This may include errors such as command line too long.
        501 Syntax error in parameters or arguments.
        502 Command not implemented.
        503 Bad sequence of commands.
        504 Command not implemented for that parameter.
        530 Not logged in.
        532 Need account for storing files.
        550 Requested action not taken.
            File unavailable (e.g., file not found, no access).
        551 Requested action aborted: page type unknown.
        552 Requested file action aborted.
            Exceeded storage allocation (for current directory or
            dataset).
        553 Requested action not taken.
            File name not allowed.

### Notes

If the Server's reply consists of multiple lines, the first will start with

    COD-...

that is, the answer code and a *dash* (*minus*), followed by user readable text; and then it will end with

    COD ...

that is, the same answer code and a *space*. So, if we don't care about the user readable text, the canonical FTP reading loop discards every line *not* like the last one.
