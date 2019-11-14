/* TCPechod.c - main, TCPechod */

#define	_USE_BSD
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		128
#define LINELEN         128

extern int	errno;

void	reaper(int);
int	TCPechod(int fd);
int	errexit(const char *format, ...);
int	passiveTCP(const char *service, int qlen);

/*------------------------------------------------------------------------
 * main - Concurrent TCP server for ECHO service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*service = "ftp";	/* service name or port number	*/
	struct	sockaddr_in fsin;	/* the address of a client	*/
	unsigned int	alen;		/* length of client's address	*/
	int	msock;			/* master server socket		*/
	int	ssock;			/* slave server socket		*/

	switch (argc) {
		case	1:
		break;
		case	2:
		service = argv[1];
		break;
		default:
		errexit("usage: TCPechod [port]\n");
	}

	msock = passiveTCP(service, QLEN);

	(void) signal(SIGCHLD, reaper);

	while (1) {
		alen = sizeof(fsin);
		ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
		if (ssock < 0) {
			if (errno == EINTR)
				continue;
			errexit("accept: %s\n", strerror(errno));
		}
		switch (fork()) {
		case 0:		/* child */
			(void) close(msock);
			printf("receive connection");
			exit(TCPechod(ssock));
		default:	/* parent */
			(void) close(ssock);
			break;
			case -1:
			errexit("fork: %s\n", strerror(errno));
		}
	}
}

/*------------------------------------------------------------------------
 * TCPechod - echo data until end of file
 *------------------------------------------------------------------------
 */
int
TCPechod(int fd)
{
	char    name[LINELEN+1];
	char	buf[BUFSIZ];
	int	cc;
	int     n;
	FILE *fp;
	if (cc = read(fd, name, sizeof name)) {
		printf("filename:%s",name);
		if (cc < 0)
			errexit("echo read: %s\n", strerror(errno));
		fp = fopen(name,"rb");
		while((n=fread(buf,sizeof(char),BUFSIZ,fp))>0){
			if(n!=BUFSIZ && ferror(fp)){
				perror("read file error");
				exit(1);
			}
			if (write(fd, buf, n) < 0)
				errexit("echo write: %s\n", strerror(errno));
			memset(buf,0,BUFSIZ);
			printf("writing");
		}
		printf("server complete");
	}
	return 0;
}

/*------------------------------------------------------------------------
 * reaper - clean up zombie children
 *------------------------------------------------------------------------
 */
void
reaper(int sig)
{
	int	status;

	while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0)
		/* empty */;
}