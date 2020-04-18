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
    long size = 0;
    long bytes_read = 0;
    long bytes_write = 0;
    long read_index = 0;
    int read_length = 0;

    char buf [BUF_SIZE] = { 0 } ;
    static const long bufsize = sizeof (buf)/sizeof(char);
    bytes_write = bufsize;
    fputs("bufsize %ld\n", stderr);

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
    char* buffer = readFileInBuffer("/opt/hgs/file.txt", &size);
    if ( buffer )
    {
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

			while( 1 )
			{
			    read_index = 0;
			    read_length = 0;
			    bytes_write = PACKAGE_SIZE;

			    read_length = (sizeof(buf) - read_index)/2;
			    gettimeofday(&st,NULL);
			    status = send(s, buffer, bytes_write, 0);
			    gettimeofday(&et,NULL);
			    int elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
			    double speed = (((double)bytes_write)/((double)elapsed));
			    fprintf(stderr, "%d:usec:%ld:bytes, %f MB/s\n",elapsed, bytes_write, speed);
			    if (status < 0) perror ("uh oh");

			    bytes_read = recv(s, buf + read_index, read_length, 0);
			    fprintf(stderr, "bytes_read %ld\n", bytes_read);
			    if( 0 < bytes_read )
			    {
				read_index = read_index + bytes_read;
			    }
			    else
			    {
				    fprintf(stderr, "TIMEOUT\n");
			    }
			    while (0 < bytes_read)
			    {
				bytes_read = 0;
				read_length = sizeof(buf) - read_index;
				if ( read_length > 0)
				{
				    bytes_read = recv(s, buf + read_index, read_length, 0);
				    fprintf(stderr, "bytes_read %ld\n", bytes_read);
				    if( 0 < bytes_read )
				    {
					read_index = read_index + bytes_read;
				    }
				}
				else
				{
				    if ( bytes_write != read_index )
				    {
					 fprintf(stderr, "Read write length ummatch bytes_write: %ld, bytes_read %ld\n", bytes_write, bytes_read);
				    }
				    if ( 0 != memcmp(buffer, buf, read_index))
				    {
					fprintf(stderr, "Buffer ummatch\n");
				    }
				    printf("received [%s]\n", buf);
				    memset(buf, 0, bufsize);
				}
			    }
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
