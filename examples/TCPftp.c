/* TCPftp.c - main, TCPftp */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int	TCPftp(const char *host, const char *service);
int	errexit(const char *format, ...);
int	connectTCP(const char *host, const char *service);

#define errno 1
#define	LINELEN		128
#define BUFSIZE        128
/*------------------------------------------------------------------------
 * main - TCP client for tiny ftp service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*service = "ftp";	/* default service name		*/
	char    *service2 = "ftp-data";
	char	filename[LINELEN+1]="";	/* buffer for one line of text	*/

	switch (argc) {
	case 3:
		host = argv[2];
	case 2:
		strncpy(filename,argv[1],strlen(argv[1]));
		break;
		/* FALL THROUGH */
	default:
		fprintf(stderr, "usage:TCPftp filename <IP> \n");
		exit(1);
	}
        printf("%s %s\n",filename,host);
	FILE *fp = fopen(filename,"wb"); /* ab */
	if(fp==NULL){
	    perror("can't open file\n");
	    exit(1);
	}

	exit(TCPfile(host,service,service2, filename,fp));
}

/*------------------------------------------------------------------------
 * TCPftp - send filename to ftp service on specified host and save file
 *------------------------------------------------------------------------
 */
void
TCPfile(const char *host, const char * service,char * service2,char * filename,FILE *fp)
{

  int	s,s1, n;			/* socket descriptor, read count*/
	int	outchars, inchars;	/* characters sent and received	*/
	char buff[BUFSIZE] ={0};
	s = connectTCP(host, service);
	s1 = connectTCP(host, service2);
	filename[LINELEN] = '\0';  /* insure line null-terminated	*/
        outchars = strlen(filename);
	(void) send(s, filename, outchars,0);
	while(n = recv(s1, buff, BUFSIZE,0)){
		if(strcmp(buff,"NO")==0){
			perror("no such file");
			exit(0);
		}
                /* printf("%s",buff); */
		/* read file back */
		if(fwrite(buff,1,n,fp)!=n){ /* 1 means how many bytes peer wrt */
		 perror("write file error\n");
		 exit(1);
		}
		if(n<BUFSIZE){
			printf("end of file\n");
			break;
		}
		memset(buff,0,BUFSIZE);
	}
	if (n < 0){
			errexit("socket read failed: %s\n",
			strerror(errno));
	}
	close(s1);
	close(s);
}
