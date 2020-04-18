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
    char buffer_recv [BUF_SIZE] = { 0 } ;
    static const long bufsize = BUF_SIZE; //sizeof (buf)/sizeof(char);
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

    ba2str(&rem_addr.rc_bdaddr, buffer_recv);
    fprintf(stderr, "accepted connection from %s\n", buffer_recv);
    memset ( buffer_recv, 0, sizeof ( buffer_recv));

    remove("out.bin");
    FILE* pFile;
    pFile = fopen("out.bin", "ab");
    if( pFile )
    { 
	    fprintf(stderr, "Opened file\n");
    }
    else
    {
	    fprintf(stderr, "Failed to open file\n");
    }
    
    long transfer_size = 0;
    long bytes_read = 0;
    long write_length = 0;
    long read_length = 0;
    long read_index = 0;
    long write_index = 0;

    while(1)
    {
        //fprintf(stderr, "read data from the client\n");
	bytes_read = 0;
        int bytes_read_temp = 0;
        read_length = PACKAGE_SIZE; //( transfer_size - read_index) > PACKAGE_SIZE ? PACKAGE_SIZE : transfer_size - read_index;
        while ( bytes_read < read_length )
        {
                fprintf(stderr, "waiting for  %ld bytes \n", read_length - bytes_read);
                //bytes_read_temp = recv(client, buffer_recv + read_index, read_length, 0);
                bytes_read_temp = recv(client, buffer_recv, read_length, 0);
                fprintf(stderr, "bytes_read %ld\n", bytes_read_temp);
                if( bytes_read_temp <= 0 )
                {
                    if ( read_index == 0 )
                    {
                        return bytes_read_temp;
                    }
                    break;
                    fprintf(stderr, "TIMEOUT\n");
                }

                read_index += bytes_read_temp;
                bytes_read += bytes_read_temp;
        }
	
        if (bytes_read > 0)
        {
		fwrite(buffer_recv, sizeof(char), bytes_read, pFile);
    		fprintf(stderr, "sending %ld bytes\n", bytes_read);
        	status = send(client, buffer_recv, bytes_read, 0);
        	fprintf(stderr, "sent %ld bytes\n", bytes_read);
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
