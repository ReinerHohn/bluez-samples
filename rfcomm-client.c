#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/time.h>

#include "common.h"

int s;
char* readFileInBuffer(char* filename, long* psize);

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
    long transfer_size = 0;
    long bytes_read = 0;
    long write_length = 0;
    long read_length = 0;
    long read_index = 0;
    long write_index = 0;


    if ( argc < 2 )
    {
        printf("Usage: client <bdaddr>");
        return 1;
    }
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
	char* buffer_recv = malloc(transfer_size + 1);

	// allocate a socket
	s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if ( -1 != s )
	{
		// set the connection parameters (who to connect to)
		addr.rc_family = AF_BLUETOOTH;
		addr.rc_channel = 1;
		fprintf(stderr, "%s\n", argv[1]);
		str2ba(argv[1], &addr.rc_bdaddr);

		// connect to server
		status = connect(s, (struct sockaddr*)&addr, sizeof(addr));
		// send a message
		if (0 == status)
		{
			struct timeval tv;
			tv.tv_sec = 10;
			tv.tv_usec = 0;
			setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

			struct timeval st, et;

			write_index = 0;
			read_index = 0;
			read_length = PACKAGE_SIZE;

			while( write_index < transfer_size )
			{
				gettimeofday(&st,NULL);
				write_length = ( transfer_size - write_index) > PACKAGE_SIZE ? PACKAGE_SIZE : transfer_size - write_index;
				fprintf(stderr, "Transferring %ld / %ld \n", write_index, transfer_size);
				status = send(s, buffer_send + write_index, write_length, 0);
				gettimeofday(&et,NULL);
				int elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
				double speed = (((double)write_length)/((double)elapsed));
				fprintf(stderr, "%d:usec:%ld:bytes, %f MB/s\n",elapsed, write_length, speed);
				if (status < 0)
				{
				    perror ("uh oh");
				    break;
				}
				write_index += write_length;

				bytes_read = 0;
				int bytes_read_temp = 0;
				read_length = ( transfer_size - read_index) > PACKAGE_SIZE ? PACKAGE_SIZE : transfer_size - read_index;
				while ( bytes_read < read_length )
				{
					fprintf(stderr, "waiting for  %ld bytes \n", read_length - bytes_read);
					bytes_read_temp = recv(s, buffer_recv + read_index, read_length, 0);
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
				if ( write_index != read_index )
				{
				     fprintf(stderr, "Read write length ummatch write_length: %ld, bytes_read %ld\n", write_length, bytes_read);
				}
				int res =  memcmp(buffer_send + write_index, buffer_recv + read_index, read_length);
				if ( res != 0 )
				{
				    //fprintf(stderr, "Buffer ummatch %d, write_index %d, read_index %d, %s %s \n", res, write_index, read_index, buffer_send + write_index, buffer_recv + read_index);
				    //fprintf(stderr, "Buffer ummatch %d, write_index %d, read_index %d, %s %s \n", res, write_index, read_index, buffer_send + write_index, buffer_recv + read_index);
				}
				//printf("received %s\n", buffer_recv);
				//memset(buffer_recv, 0, bufsize);
			}
		}
		close(s);
	}
	else
	{
		perror("Failed to open socket\n");
	}
	return 0;
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
