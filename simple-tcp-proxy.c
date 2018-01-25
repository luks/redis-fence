/*
 * $Id: simple-tcp-proxy.c,v 1.11 2006/08/03 20:30:48 wessels Exp $
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <syslog.h>
#include <err.h>


#include <sys/types.h>
#include <sys/select.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>




///#define BUF_SIZE 4096
#define BUF_SIZE (1024*10)



#define REDIS_STAR  "*"

#define REDIS_DOLLAR  "$"

#define REDIS_CR  "\r"

#define REDIS_LF  "\n"

//move to header latter

#ifndef _REQUEST_H
#define _REQUEST_H

struct request{
    char *querybuf;
    int argc;
    char **argv;
    size_t pos;
};

struct request *request_new(char *querybuf);
int  request_parse(struct request *request);
void request_free(struct request *request);
void request_dump(struct request *request);

#endif

// header
// request methods



enum {
    STATE_CONTINUE,
    STATE_FAIL
};

struct request *request_new(char *querybuf) {
    struct request *req;
    req=calloc(1, sizeof(struct request));
    req->querybuf = querybuf;
    return req;
}


int req_state_len(struct request *req,char *sb) {
    int term=0, first=0;
    char c;
    int i = req->pos;
    int pos=i;

    while((c = req->querybuf[i]) != '\0') {
        first++;
        pos++;
        switch(c) {
            case '\r':
                term = 1;
                break;

            case '\n':
                if (term) {
                    req->pos = pos;
                    return STATE_CONTINUE;
                }
                else
                    return STATE_FAIL;
            default:
                if (first == 1) {
                    /* the first symbol is not '*' or '$'*/
                    if (c != *REDIS_DOLLAR && c != *REDIS_STAR && c != *REDIS_CR && c != *REDIS_LF)
                        return STATE_FAIL;
                } else {
                    /* the symbol must be numeral*/
                    if (c >= '0' && c <= '9')
                        *sb++ = c;
                    else
                        return STATE_FAIL;
                }
                break;
        }
        i++;
    }

    return STATE_FAIL;
}


int request_parse(struct request *req) {
    int i;
    char sb[BUF_SIZE] = {0};

    if (req_state_len(req, sb) != STATE_CONTINUE) {
        fprintf(stderr, "argc format ***ERROR***,packet:%s", sb);
        return 0;
    }
    req->argc = atoi(sb);

    req->argv  =(char**)calloc(req->argc, sizeof(char*));
    for (i = 0; i < req->argc; i++) {
        int argv_len;
        char *v;

        /*parse argv len*/
        memset(sb, 0, BUF_SIZE);
        if (req_state_len(req, sb) != STATE_CONTINUE) {
            fprintf(stderr, "argv's length format ***ERROR***,packet:%s", sb);
            return 0;
        }
        argv_len=atoi(sb);

        /*get argv*/
        v=(char*)malloc(sizeof(char) * argv_len);
        memcpy(v, req->querybuf+(req->pos), argv_len);
        req->argv[i] = v;
        req->pos += argv_len+2;
    }
    return 1;
}

void request_dump(struct request *req) {
    int i;
    if (req == NULL)
        return;

    printf("request-dump--->");
    printf("argc:<%d>\n", req->argc);
    for (i = 0; i < req->argc; i++) {
        printf("		argv[%d]:<%s>\n",i,req->argv[i]);
    }
    printf("\n");
}

void request_free(struct request *req) {
    int i;
    if (req) {
        for (i = 0; i < req->argc; i++) {
            if (req->argv[i])
                free(req->argv[i]);
        }
        free(req->argv);
        free(req);
    }
}
// end of request methods

void redis_command_from_buffer(char * cbuf, int n) {

    int ret;
    struct request *req=request_new(cbuf);
    ret=request_parse(req);
    if(ret) {
//        request_dump(req);
        printf("%s\n", req->argv[0]);

    }

//    if (n > 0)
//    {
//        printf("%s\n", cbuf);
//
//        int i;
//        for (i = 0; i < n; ++i)
//        {
////            if (cbuf[0] == *REDIS_STAR) {
////               i++;
////            }
//
//            if (cbuf[i] == *REDIS_STAR) {
//                i++;
//            }
//
//            if (cbuf[i] == *REDIS_CR) {
//                i++;
//            }
//
//            if (cbuf[i] == *REDIS_LF) {
//                i++;
//            }
//
//            if (cbuf[i] == *REDIS_DOLLAR) {
//                i++;
//            }
//            printf("%c\n", cbuf[i]);
//        }
//
//    }

}


// *2\r\n
// $4\r\n
// keys\r\n
// $1\r\n
// *\r\n




char client_hostname[64];

void
cleanup(int sig)
{
    syslog(LOG_NOTICE, "Cleaning up...");
    exit(0);
}

void
sigreap(int sig)
{
    int status;
    pid_t p;
    signal(SIGCHLD, sigreap);
    while ((p = waitpid(-1, &status, WNOHANG)) > 0);
    /* no debugging in signal handler! */
}

void
set_nonblock(int fd)
{
    int fl;
    int x;
    fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0) {
	syslog(LOG_ERR, "fcntl F_GETFL: FD %d: %s", fd, strerror(errno));
	exit(1);
    }
    x = fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    if (x < 0) {
	syslog(LOG_ERR, "fcntl F_SETFL: FD %d: %s", fd, strerror(errno));
	exit(1);
    }
}


int
create_server_sock(char *addr, int port)
{
    int addrlen, s, on = 1, x;
    static struct sockaddr_in client_addr;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
	err(1, "socket");

    addrlen = sizeof(client_addr);
    memset(&client_addr, '\0', addrlen);
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(addr);
    client_addr.sin_port = htons(port);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, 4);
    x = bind(s, (struct sockaddr *) &client_addr, addrlen);
    if (x < 0)
	err(1, "bind %s:%d", addr, port);

    x = listen(s, 5);
    if (x < 0)
	err(1, "listen %s:%d", addr, port);
    syslog(LOG_NOTICE, "listening on %s port %d", addr, port);

    return s;
}

int
open_remote_host(char *host, int port)
{
    struct sockaddr_in rem_addr;
    int len, s, x;
    struct hostent *H;
    int on = 1;

    H = gethostbyname(host);
    if (!H)
	return (-2);

    len = sizeof(rem_addr);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
	return s;

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, 4);

    len = sizeof(rem_addr);
    memset(&rem_addr, '\0', len);
    rem_addr.sin_family = AF_INET;
    memcpy(&rem_addr.sin_addr, H->h_addr, H->h_length);
    rem_addr.sin_port = htons(port);
    x = connect(s, (struct sockaddr *) &rem_addr, len);
    if (x < 0) {
	close(s);
	return x;
    }
    set_nonblock(s);
    return s;
}

int
get_hinfo_from_sockaddr(struct sockaddr_in addr, int len, char *fqdn)
{
    struct hostent *hostinfo;

    hostinfo = gethostbyaddr((char *) &addr.sin_addr.s_addr, len, AF_INET);
    if (!hostinfo) {
	sprintf(fqdn, "%s", inet_ntoa(addr.sin_addr));
	return 0;
    }
    if (hostinfo && fqdn)
	sprintf(fqdn, "%s [%s]", hostinfo->h_name, inet_ntoa(addr.sin_addr));
    return 0;
}


int
wait_for_connection(int s)
{
    static int newsock;
    static socklen_t len;
    static struct sockaddr_in peer;

    len = sizeof(struct sockaddr);
    syslog(LOG_INFO, "calling accept FD %d", s);
    newsock = accept(s, (struct sockaddr *) &peer, &len);
    /* dump_sockaddr (peer, len); */
    if (newsock < 0) {
	if (errno != EINTR) {
	    syslog(LOG_NOTICE, "accept FD %d: %s", s, strerror(errno));
	    return -1;
	}
    }
    get_hinfo_from_sockaddr(peer, len, client_hostname);
    set_nonblock(newsock);
    return (newsock);
}

int
mywrite(int fd, char *buf, int *len)
{
	int x = write(fd, buf, *len);
	if (x < 0)
		return x;
	if (x == 0)
		return x;
	if (x != *len)
		memmove(buf, buf+x, (*len)-x);
	*len -= x;
	return x;
}

void
service_client(int cfd, int sfd)
{
    int maxfd;
    char *sbuf;
    char *cbuf;
    int x, n;
    int cbo = 0;
    int sbo = 0;
    fd_set R;

    sbuf = malloc(BUF_SIZE);
    cbuf = malloc(BUF_SIZE);
    maxfd = cfd > sfd ? cfd : sfd;
    maxfd++;

    while (1) {
	struct timeval to;
	if (cbo) {
		if (mywrite(sfd, cbuf, &cbo) < 0 && errno != EWOULDBLOCK) {
			syslog(LOG_ERR, "write %d: %s", sfd, strerror(errno));
				exit(1);
		}
	}
	if (sbo) {
		if (mywrite(cfd, sbuf, &sbo) < 0 && errno != EWOULDBLOCK) {
			syslog(LOG_ERR, "write %d: %s", cfd, strerror(errno));
				exit(1);
		}
	}
	FD_ZERO(&R);
	if (cbo < BUF_SIZE)
		FD_SET(cfd, &R);
	if (sbo < BUF_SIZE)
		FD_SET(sfd, &R);
	to.tv_sec = 0;
	to.tv_usec = 1000;
	x = select(maxfd+1, &R, 0, 0, &to);
	if (x > 0) {
	    if (FD_ISSET(cfd, &R)) {
		n = read(cfd, cbuf+cbo, BUF_SIZE-cbo);

        redis_command_from_buffer(cbuf, n);

        syslog(LOG_INFO, "read %d bytes from CLIENT (%d)", n, cfd);
		if (n > 0) {
		    cbo += n;
		} else {
		    close(cfd);
		    close(sfd);
		    syslog(LOG_INFO, "exiting");
		    _exit(0);
		}
	    }
	    if (FD_ISSET(sfd, &R)) {
		n = read(sfd, sbuf+sbo, BUF_SIZE-sbo);
		syslog(LOG_INFO, "read %d bytes from SERVER (%d)", n, sfd);
		if (n > 0) {
		    sbo += n;
		} else {
		    close(sfd);
		    close(cfd);
		    syslog(LOG_INFO, "exiting");
		    _exit(0);
		}
	    }
	} else if (x < 0 && errno != EINTR) {
	    syslog(LOG_NOTICE, "select: %s", strerror(errno));
	    close(sfd);
	    close(cfd);
	    syslog(LOG_NOTICE, "exiting");
	    _exit(0);
	}
    }
}


int
main(int argc, char *argv[])
{
    char *localaddr = NULL;
    int localport = -1;
    char *remoteaddr = NULL;
    int remoteport = -1;
    int client = -1;
    int server = -1;
    int master_sock = -1;

    if (5 != argc) {
	fprintf(stderr, "usage: %s laddr lport rhost rport\n", argv[0]);
	exit(1);
    }

    localaddr = strdup(argv[1]);
    localport = atoi(argv[2]);
    remoteaddr = strdup(argv[3]);
    remoteport = atoi(argv[4]);

    assert(localaddr);
    assert(localport > 0);
    assert(remoteaddr);
    assert(remoteport > 0);

    openlog(argv[0], LOG_PID, LOG_LOCAL4);

    signal(SIGINT, cleanup);
    signal(SIGCHLD, sigreap);

    master_sock = create_server_sock(localaddr, localport);
    for (;;) {
	if ((client = wait_for_connection(master_sock)) < 0)
	    continue;
	if ((server = open_remote_host(remoteaddr, remoteport)) < 0) {
	    close(client);
	    client = -1;
	    continue;
	}
	if (0 == fork()) {
	    /* child */
	    syslog(LOG_NOTICE, "connection from %s fd=%d", client_hostname, client);
	    syslog(LOG_INFO, "connected to %s:%d fd=%d", remoteaddr, remoteport, server);
	    close(master_sock);
	    service_client(client, server);
	    abort();
	}
	close(client);
	client = -1;
	close(server);
	server = -1;
    }

}
