#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>

#include "dbus-bt.h" 

#include "common.h"

int fd_sock_rfcomm, fd_sock_l2cap, client;

static void catch_function(int signo)
{
    puts("Interactive attention signal caught.");
    close(client);
    close(fd_sock_rfcomm);

    exit(1);
}

int getRfcommSocket()
{
    char buf[1024] = { 0 };
    struct sockaddr_rc loc_addr_rfcomm = { 0 } , rem_addr_rfcomm = { 0 } ;
    unsigned int opt_rfcomm = sizeof (rem_addr_rfcomm);

    // allocate socket
    fd_sock_rfcomm = socket (AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // bind socket to port 1 of the first available bluetooth adapter
    loc_addr_rfcomm.rc_family = AF_BLUETOOTH ;
    loc_addr_rfcomm.rc_bdaddr = *BDADDR_ANY ;
    loc_addr_rfcomm.rc_channel =1;
    bind(fd_sock_rfcomm, (struct sockaddr *)&loc_addr_rfcomm, sizeof(loc_addr_rfcomm));

    // put socket into listening mode
    listen(fd_sock_rfcomm, 1);

    // accept one connection
    client = accept(fd_sock_rfcomm, (struct sockaddr *)&rem_addr_rfcomm, &opt_rfcomm);

    ba2str(&rem_addr_rfcomm.rc_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);

    return client;
}

int getl2CapSocket(uint16_t mtu)
{
    char buf[1024] = { 0 };
    struct sockaddr_l2 loc_addr_l2 = { 0 },  rem_addr_l2 = { 0 };
    unsigned int opt_l2 = sizeof (rem_addr_l2);

    fd_sock_l2cap = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    loc_addr_l2.l2_family = AF_BLUETOOTH;
    loc_addr_l2.l2_bdaddr = *BDADDR_ANY;
    loc_addr_l2.l2_psm = htobs(0x1001);

    set_l2cap_mtu( fd_sock_l2cap, mtu );

    bind(fd_sock_l2cap, (struct sockaddr *)&loc_addr_l2, sizeof(loc_addr_l2));

    // put socket into listening mode
    listen(fd_sock_l2cap, 1);

    // accept one connection
    client = accept(fd_sock_l2cap, (struct sockaddr *)&rem_addr_l2, &opt_l2);
    ba2str( &rem_addr_l2.l2_bdaddr, buf );
    fprintf(stderr, "accepted connection from %s\n", buf);

    return client;
}
int main (int argc, char** argv)
{
    char buffer_recv [BUF_SIZE] = { 0 } ;
    static const long bufsize = BUF_SIZE; //sizeof (buf)/sizeof(char);
    int status;

    if ( argc < 2 )
    {
        printf("Usage: client <package_size>");
        return 1;
    }
    char* p;
    long packageSize = strtol(argv[1], &p, 10);

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
#if defined USE_L2CAP
    client = getl2CapSocket(packageSize);
#else
    client = getRfcommSocket();
#endif

    long transfer_size = 0;
    long bytes_read = 0;
    long write_length = 0;
    long read_length = 0;
    long read_index = 0;
    long write_index = 0;

    fd_set read_sd;
    FD_ZERO(&read_sd);
    FD_SET(client, &read_sd);
    //int packageSize = 512;
    fprintf(stderr, "Working with paket size %ld\n", packageSize);
    while(1)
    {

        bytes_read = 0;
        int bytes_read_temp = 0;
        read_length = packageSize; //( transfer_size - read_index) > packageSize ? packageSize : transfer_size - read_index;

        while ( bytes_read < read_length )
        {
            //fprintf(stderr, "waiting for  %ld bytes \n", read_length - bytes_read);
            //bytes_read_temp = recv(client, buffer_recv + read_index, read_length, 0);

            fd_set rsd = read_sd;
            int sel = select(client + 1, &rsd, 0, 0, 0);
            if (sel > 0)
            {
                bytes_read_temp = recv(client, buffer_recv, read_length, 0);
                if ( 0 == bytes_read_temp)
                {
                    fprintf(stderr, "Client disconnected\n");
                    return 0;
                }
                //fprintf(stderr, "bytes_read %ld\n", bytes_read_temp);
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
            else if (sel < 0)
            {
                // grave error occurred.
                break;
            }
        }
        if (bytes_read > 0)
        {
            //fwrite(buffer_recv, sizeof(char), bytes_read, pFile);
            //printf(stderr, "sending %ld bytes\n", bytes_read);
            status = send(client, buffer_recv, bytes_read, 0);
            //fprintf(stderr, "sent %ld bytes\n", bytes_read);
        }
        else
        {
            //fprintf(stderr, "no bytes received \n");
            perror("Error receving bytes:");
        }
    }


    // close connection
    close(client);
    close(fd_sock_rfcomm);
    return 0;
}
