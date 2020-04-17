#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

int s;

static void catch_function(int signo)
{
    puts("Interactive attention signal caught.");
    close(s);

    exit(1);
}

int main (int argc, char** argv)
{
    struct sockaddr_rc addr = { 0 } ;
    int status;
    char dest[18] = "B8:27:EB:7D:19:FA";

    if (signal(SIGINT, catch_function) == SIG_ERR)
    {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        //return EXIT_FAILURE;
    }
    if (signal(SIGKILL, catch_function) == SIG_ERR)
    {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        //return EXIT_FAILURE;
    }

    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = 1;
    str2ba(dest, &addr.rc_bdaddr);

    // connect to server
    status = connect(s, (struct sockaddr*)&addr, sizeof(addr));

    // send a message
    if (0 == status) {
        status = send(s, "hello!", 6, 0);
    }

    if (status < 0) perror ("uh oh");

    close(s);
    return 0;
}
