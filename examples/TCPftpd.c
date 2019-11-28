/* TCPftpd.c - main, TCPftpd */

#define	_USE_BSD
#include <sys/types.h>
#include <sys/signal.h>
#include <bits/sigaction.h>
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

#define	QLEN		128	/* maximum connection queue length	*/
#define	BUFSIZE		128
#define LINELEN         128

extern int	errno;

void	reaper(int);
void    sig_zchild(int);
void	ouch(int);
void    sigg(int);
int	TCPftpd(int fd,int fd2);
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
	char    *service2 = "ftp-data";   /* service name2 */
	struct	sockaddr_in fsin,fsin2;	/* the address of a client	*/
	unsigned int	alen,alen2;		/* length of client's address	*/
	int	msock,msock2;			/* master server socket		*/
	int	ssock,ssock2;			/* slave server socket		*/

	switch (argc) {
		case	1:
		break;
		case	2:
		service = argv[1];
		break;
		default:
		errexit("usage: TCPftpd [port]\n");
	}

	msock = passiveTCP(service, QLEN); /* return a bind& listening socket on 21*/
	msock2 = passiveTCP(service2, QLEN); /*return a bind& listening socket on 20 */
	printf("msock %d\n",msock);
	printf("msock2 %d\n",msock2);

/*	(void) signal(SIGCHLD, sig_zchild); */
	signal(SIGCHLD, SIG_IGN);
/*	struct sigaction act;
	act.sa_handler = sigg;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGCHLD,&act,0); */

/*default*/

	while (1) {
		alen = sizeof(fsin);
		alen2 = sizeof(fsin2);
		ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
		if (ssock <0){
			errexit("accept: %s\n", strerror(errno));
		}
		printf("ssock accepted\n");
	      ssock2 = accept(msock2, (struct sockaddr *)&fsin2,&alen2);
		if( ssock2 < 0){
			errexit("accept2: %s\n", strerror(errno));
		}
		printf("ssock2 accepted\n");
		switch (fork()) {
		case 0:		/* child */
			close(msock);
			close(msock2);
			printf("connection received\n");
			TCPftpd(ssock,ssock2);
			exit(0);
		default:	/* parent */
			close(ssock);
			close(ssock2);
			break;
		case -1:
			errexit("fork: %s\n", strerror(errno));
		}
	}
	close(msock);
}

/*------------------------------------------------------------------------
 * TCPftpd - receive a filename and send it back
 *------------------------------------------------------------------------
 */
int
TCPftpd(int fd,int fd2)
{
	char    name[LINELEN+1];
	char	buf[BUFSIZ];
	char	cancel[2]="NO";
	int	cc;
	int     n;
	FILE *fp;
	if (cc = read(fd, name, sizeof name)) { /*get filename*/
		printf("filename:%s\n",name);
		if (cc < 0)
			errexit("echo read: %s\n", strerror(errno));
		fp = fopen(name,"rb"); /*open file*/
                if( NULL== fp)
		{
			printf("fopen failed\n");
			write(fd2,cancel,2);
			return;
		}
		/*
		fseek(fp,0,SEEK_END);
		long size = ftell(fp);
		rewind(fp);
		char* buffer = (char*)malloc(sizeof(char) * size);
		if( NULL == buffer){
			printf("malloc failed");
			return ;
		}
		*/
		while((n=fread(buf,1,BUFSIZ,fp))>0){
			/*if (write(fd2, buf, BUFSIZ) < 0)*/
			if(send(fd2,buf,n,0)<0)
				errexit("echo write: %s\n", strerror(errno));
			memset(buf,0,BUFSIZ);
		/*	printf("writing...\n"); */
		}
		printf("server complete\n");
	}
        close(fp); /* close file */
	close(fd); /* close socket */
	close(fd2);
	printf("child should closed");
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
	  printf("child process exit with %d",status);
}
void sig_zchild(int sig)
{
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1,&stat, WNOHANG)) > 0)
        printf("child %d terminated\n",pid);
    printf("sig_zchild exit");
}
void sigg(int sig)
{
    pid_t pid;
    int stat;
    pid = waitpid(-1,&stat,WNOHANG);
    printf("child %d die\n",pid);
}
void ouch(int sig){
	printf("child got signal %d\n",sig);
	wait(NULL);
}
