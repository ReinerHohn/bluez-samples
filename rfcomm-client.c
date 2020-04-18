#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>
#include <sys/time.h>

#include "common.h"

int g_socket;
int g_status;
char* readFileInBuffer(char* filename, long* psize);

static void catch_function(int signo)
{
    puts("Interactive attention signal caught.");
    close(g_socket);

    exit(1);
}
int getl2CapSocket(char* bdaddr, uint16_t mtu)
{
    int fd_sock_l2cap;
    struct sockaddr_l2 addr_l2cap = { 0 };

    fd_sock_l2cap = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if ( -1 != fd_sock_l2cap )
    {
        // set the connection parameters (who to connect to)
        addr_l2cap.l2_family = AF_BLUETOOTH;
        addr_l2cap.l2_psm = htobs(0x1001);
        str2ba( bdaddr, &addr_l2cap.l2_bdaddr );

        set_l2cap_mtu( fd_sock_l2cap, mtu );
        // connect to server
        g_status = connect(fd_sock_l2cap, (struct sockaddr *)&addr_l2cap, sizeof(addr_l2cap));
    }
    else
    {
        perror("Failed to open socket\n");
    }
    return fd_sock_l2cap;
}
int getRfcommSocket(char* bdaddr)
{

    int fd_sock_rfcomm;
    struct sockaddr_rc addr_rfcomm = { 0 };

    // allocate a socket
    fd_sock_rfcomm = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    if ( -1 != fd_sock_rfcomm )
    {
        // set the connection parameters (who to connect to)
        addr_rfcomm.rc_family = AF_BLUETOOTH;
        addr_rfcomm.rc_channel = 1;
        fprintf(stderr, "%s\n", bdaddr);
        str2ba(bdaddr, &addr_rfcomm.rc_bdaddr);

        // connect to server
        g_status = connect(fd_sock_rfcomm, (struct sockaddr*)&addr_rfcomm, sizeof(addr_rfcomm));
    }
    else
    {
        perror("Failed to open socket\n");
    }
    return fd_sock_rfcomm;
}
#include <stdio.h>

int sendRecFile(char* buffer_send, long transfer_size, char* filename, long packageSize)
{
    long bytes_read = 0;
    long write_length = 0;
    long read_length = 0;
    long read_index = 0;
    long write_index = 0;

    char* buffer_recv = malloc(transfer_size + 1);
    struct timeval st, et;

    char data[1024];

    FILE* pFile;
    pFile = fopen(filename, "at");
    if( pFile )
    {
        fprintf(stderr, "Opened file\n");
    }
    else
    {
        fprintf(stderr, "Failed to open file\n");
    }
    write_index = 0;
    read_index = 0;
    read_length = packageSize;

    gettimeofday(&st,NULL);
    fprintf(stderr, "Transferring %ld bytes at package size %ld\n", transfer_size, packageSize);
    while( write_index < transfer_size )
    {
        //gettimeofday(&st,NULL);
        write_length = ( transfer_size - write_index) > packageSize ? packageSize : transfer_size - write_index;
        //fprintf(stderr, "Transferring %ld / %ld \n", write_index, transfer_size);
        g_status = send(g_socket, buffer_send + write_index, write_length, 0);
        /*gettimeofday(&et,NULL);
        int elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);*/
        //double speed = (((double)write_length)/((double)elapsed));
        //speedsum += speed;
        //fprintf(stderr, "%d:usec:%ld:bytes, %f MB/s\n",elapsed, write_length, speed);
        //fprintf(stderr, "%f MB/s av\n",speedsum/counter);
        //counter++;
        if (g_status < 0)
        {
            perror ("uh oh");
            break;
        }
        write_index += write_length;
#if 1//READ_BACK
        bytes_read = 0;
        int bytes_read_temp = 0;
        read_length = ( transfer_size - read_index) > packageSize ? packageSize : transfer_size - read_index;
        while ( bytes_read < read_length )
        {
            //fprintf(stderr, "waiting for  %ld bytes \n", read_length - bytes_read);
            bytes_read_temp = recv(g_socket, buffer_recv + read_index, read_length, 0);
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
        if ( write_index != read_index )
        {
             //fprintf(stderr, "Read write length ummatch write_length: %ld, bytes_read %ld\n", write_length, bytes_read);
        }
        //int res =  memcmp(buffer_send + write_index, buffer_recv + read_index, read_length);
        //if ( res != 0 )
        {
            //fprintf(stderr, "Buffer ummatch %d, write_index %d, read_index %d, %s %s \n", res, write_index, read_index, buffer_send + write_index, buffer_recv + read_index);
            //fprintf(stderr, "Buffer ummatch %d, write_index %d, read_index %d, %s %s \n", res, write_index, read_index, buffer_send + write_index, buffer_recv + read_index);
        }
        //printf("received %s\n", buffer_recv);
        //memset(buffer_recv, 0, bufsize);
#endif
    }
    gettimeofday(&et,NULL);
    int elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);

    sprintf(data, "Transferred:%ld: bytes in :%d: s at package size :%ld: so %f kbit/s\n", transfer_size, elapsed/ 1000000, packageSize, ((double)transfer_size/(double)elapsed)*1000);
    fwrite(data, sizeof(char), strlen(data), pFile); //sizeof(data)/sizeof (char)
    fprintf(stderr, "%s\n", data);
    fclose(pFile);
    free(buffer_recv);
}

int main (int argc, char** argv)
{
    long transfer_size = 0;
    if ( argc < 3 )
    {
        printf("Usage: client <bdaddr> <package_size>");
        return 1;
    }
    char filename[50];
    char* p;
    long packageSize = strtol(argv[2], &p, 10);

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
    char* buffer_send = readFileInBuffer("/opt/hgs/file.txt", &transfer_size);
    if ( buffer_send )
    {

#if defined USE_L2CAP
         g_socket = getl2CapSocket(argv[1], packageSize);
#else
         g_socket = getRfcommSocket(argv[1]);
#endif
		// send a message
        if (0 == g_status)
		{
			struct timeval tv;
            tv.tv_sec = 1;
			tv.tv_usec = 0;
            setsockopt(g_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

            //for(int packageSize = 512; packageSize < 4096; packageSize++)
            {
                //sprintf(filename, "outfile%d", packageSize);
                sendRecFile(buffer_send, transfer_size, "outfile", packageSize);
            }
		}
        else
        {
            perror( "Could not connect\n" );
            fprintf(stderr, "Server address was: %s\n", argv[1]);
        }

        close(g_socket);
   }
   else
   {
	   fputs("Could not read /opt/hgs/file.txt\n", stderr);
   }
}

char* readFileInBuffer(char* filename, long* psize)
{
    char *string = NULL;
    FILE *f = fopen(filename, "rb");
    if ( f )
    {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        *psize = fsize;
        fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

        string = malloc(fsize + 1);
        fread(string, 1, fsize, f);
        fclose(f);

        string[fsize] = 0;
    }

    return string;
}
