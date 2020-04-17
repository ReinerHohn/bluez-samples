#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "dbus-bt.h" 

#include "common.h"

int s, client;

static void catch_function(int signo)
{
    puts("Interactive attention signal caught.");
    close(client);
    close(s);

    exit(1);
}

int main (int argc, char** argv)
{
    struct sockaddr_rc loc_addr = { 0 } , rem_addr = { 0 } ;
    char buf [BUF_SIZE] = { 0 } ;
    static const long bufsize = sizeof (buf)/sizeof(char);
    int bytes_read;
    int status;
    unsigned int opt = sizeof (rem_addr);

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

    init_bt();

    // allocate socket
    s = socket (AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // bind socket to port 1 of the first available bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH ;
    loc_addr.rc_bdaddr = *BDADDR_ANY ;
    loc_addr.rc_channel =1;
    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

    // put socket into listening mode
    listen(s, 1);

    // accept one connection
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);

    ba2str(&rem_addr.rc_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset ( buf ,0, sizeof ( buf));

    while(1)
    {
        //fprintf(stderr, "read data from the client\n");
        bytes_read = recv(client, buf, sizeof(buf), 0);
        if (bytes_read > 0)
        {
            fprintf(stderr, "received [%s]\n", buf);
            //status = send(client, buf, bytes_read, 0);
        }
        else
        {
            //fprintf(stderr, "no bytes received \n");
            perror("Error receving bytes:");
        }
    }
    // close connection
    close(client);
    close(s);
    return 0;
}
