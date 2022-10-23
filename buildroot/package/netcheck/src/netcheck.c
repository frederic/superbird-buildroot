#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <getopt.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#define FIFO_NAME "/tmp/netfifo"

#define     MAXWAIT 10
#define     MAXPACKET 4096
#define     VERBOSE  1
#define     QUIET  2
#define     FLOOD  4
#ifndef     MAXHOSTNAMELEN
#define     MAXHOSTNAMELEN 64
#endif

u_char    packet[MAXPACKET];
int    i, pingflags, options;
extern    int errno;

int s;
struct hostent *hp;
struct timezone tz;

struct sockaddr whereto;
int datalen;

char usage[] =
"Usage:  ping [-dfqrv] host [packetsize [count [preload]]]\n";

char *hostname;
char hnamebuf[MAXHOSTNAMELEN];

int npackets;
int preload = 0;
int ntransmitted = 0;
int ident;

int nreceived = 0;
int timing = 0;
int tmin = 999999999;
int tmax = 0;
int tsum = 0;

void finish(int sig);
void catcher(int sig);
char *inet_ntoa();
void pinger(void);

main(argc, argv)
char *argv[];
{
    pid_t mpid;
    int ret_val=0;
    char buf[100]={0};
    unlink(FIFO_NAME);
    if ((mkfifo(FIFO_NAME,0777)<0) && (errno != EEXIST))
    {
        printf("cannot create fifo...\n");
        exit(1);
    }

    mpid=fork();
    if (mpid == 0) {
        int len = sizeof (packet);
        struct sockaddr_in from;
        int fromlen = sizeof (from);
        int cc;
        struct timeval timeout;
        int fdmask = 1 << s;
        char **av = argv;
        struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
        int on = 1;
        struct protoent *proto;
        int w_num;
        int fd;
        argc--, av++;
        while (argc > 0 && *av[0] == '-') {
            while (*++av[0]) switch (*av[0]) {
                case 'd':
                    options |= SO_DEBUG;
                    break;
                case 'r':
                    options |= SO_DONTROUTE;
                    break;
                case 'v':
                    pingflags |= VERBOSE;
                    break;
                case 'q':
                    pingflags |= QUIET;
                    break;
                case 'f':
                    pingflags |= FLOOD;
                    break;
            }
            argc--, av++;
        }
        if (argc < 1 || argc > 4)  {
            //printf(usage);
            exit(1);
        }

        bzero((char *)&whereto, sizeof(struct sockaddr) );
        to->sin_family = AF_INET;
        to->sin_addr.s_addr = inet_addr(av[0]);
        if (to->sin_addr.s_addr != (unsigned)-1) {
            strcpy(hnamebuf, av[0]);
            hostname = hnamebuf;
        } else {
            hp = gethostbyname(av[0]);
            if (hp) {
                to->sin_family = hp->h_addrtype;
                bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
                hostname = hp->h_name;
            } else {
               exit(1);
            }
        }

        if ( argc >= 2 )
            datalen = atoi( av[1] );
        else
            datalen = 64-8;
        if (datalen > MAXPACKET) {
            fprintf(stderr, "ping: packet size too large\n");
            exit(1);
        }
        if (datalen >= sizeof(struct timeval))    /* can we time 'em? */
            timing = 1;

        if (argc >= 3)
            npackets = atoi(av[2]);

        if (argc == 4)
            preload = atoi(av[3]);

        ident = getpid() & 0xFFFF;


        if ((proto = getprotobyname("icmp")) == NULL) {
            fprintf(stderr, "icmp: unknown protocol\n");
            exit(10);
        }

        if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
            perror("ping: socket");
            exit(5);
        }
        if (options & SO_DEBUG) {
            if (pingflags & VERBOSE)
            printf("...debug on.\n");
            setsockopt(s, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));
        }
        if (options & SO_DONTROUTE) {
            if (pingflags & VERBOSE)
                 printf("...no routing.\n");
            setsockopt(s, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));
        }

        if (to->sin_family == AF_INET) {
            //printf("PING %s (%s): %d data bytes\n", hostname,
            //inet_ntoa(to->sin_addr), datalen);    /* DFM */
        } else {
           //printf("PING %s: %d data bytes\n", hostname, datalen );
        }
        setlinebuf( stdout );

        signal( SIGINT, finish );
        signal(SIGALRM, catcher);

        for (i=0; i < preload; i++)
             pinger();

        if (!(pingflags & FLOOD))
             catcher(0);

        fd=open(FIFO_NAME,O_WRONLY|O_NONBLOCK);

        while (1) {

        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;

        if (pingflags & FLOOD) {
            pinger();
            if ( select(32, &fdmask, 0, 0, &timeout) == 0)
                continue;
        }
        if ( (cc=recvfrom(s, packet, len, 0, &from, &fromlen)) < 0) {
            if ( errno == EINTR )
                continue;
            perror("ping: recvfrom");
            continue;
        }
        pr_pack( packet, cc, &from );
        w_num=write(fd,"netconect\0",10);
        if (npackets && nreceived >= npackets)
            finish(0);
        }
    } else {
        int times=0;
        int num=0;
        char r_buf[50];
        int  fd;
        int  r_num;
        fd=open(FIFO_NAME,O_RDONLY|O_NONBLOCK);
        if (fd == -1)
        {
            printf("open %s for read error\n");
            exit(1);
        }
        while (1) {

            memset(r_buf,0,50);
            r_num=read(fd,r_buf,10);
            if (r_num == 10) times++;
            sleep(1);
            num++;
            if ((num>12) && (times>3)) {
                ret_val=1;
            } else ret_val=0;

            if (num>12) {
                kill(mpid,SIGKILL);
                unlink(FIFO_NAME);
                break;
            }
        }
    }
    return ret_val;
}


void catcher(int sig)
{
    int waittime;

    pinger();
    if (npackets == 0 || ntransmitted < npackets)
        alarm(1);
    else {
        if (nreceived) {
            waittime = 2 * tmax / 1000;
            if (waittime == 0)
                waittime = 1;
        } else
            waittime = MAXWAIT;
        signal(SIGALRM, finish);
        alarm(waittime);
    }
}


void pinger(void)
{
    static u_char outpack[MAXPACKET];
    register struct icmp *icp = (struct icmp *) outpack;
    int i, cc;
    register struct timeval *tp = (struct timeval *) &outpack[8];
    register u_char *datap = &outpack[8+sizeof(struct timeval)];

    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = ntransmitted++;
    icp->icmp_id = ident;

    cc = datalen+8;

    if (timing)
        gettimeofday( tp, &tz );

    for ( i=8; i<datalen; i++)
        *datap++ = i;

    icp->icmp_cksum = in_cksum( icp, cc );

    i = sendto( s, outpack, cc, 0, &whereto, sizeof(struct sockaddr) );

    if ( i < 0 || i != cc )  {
        //if( i<0 )  perror("sendto");
        //printf("ping: wrote %s %d chars, ret=%d\n",
        //    hostname, cc, i );
        //fflush(stdout);
    }
    if (pingflags == FLOOD) {
        putchar('.');
        fflush(stdout);
    }
}


char *
pr_type( t )
register int t;
{
    static char *ttab[] = {
        "Echo Reply",
        "ICMP 1",
        "ICMP 2",
        "Dest Unreachable",
        "Source Quench",
        "Redirect",
        "ICMP 6",
        "ICMP 7",
        "Echo",
        "ICMP 9",
        "ICMP 10",
        "Time Exceeded",
        "Parameter Problem",
        "Timestamp",
        "Timestamp Reply",
        "Info Request",
        "Info Reply"
    };

    if ( t < 0 || t > 16 )
        return("OUT-OF-RANGE");

    return(ttab[t]);
}


pr_pack( buf, cc, from )
char *buf;
int cc;
struct sockaddr_in *from;
{
    struct ip *ip;
    register struct icmp *icp;
    register long *lp = (long *) packet;
    register int i;
    struct timeval tv;
    struct timeval *tp;
    int hlen, triptime;

    from->sin_addr.s_addr = ntohl( from->sin_addr.s_addr );
    gettimeofday( &tv, &tz );

    ip = (struct ip *) buf;
    hlen = ip->ip_hl << 2;
    if (cc < hlen + ICMP_MINLEN) {
        //if (pingflags & VERBOSE)
        //printf("packet too short (%d bytes) from %s\n", cc,
        //inet_ntoa(ntohl(from->sin_addr))); /* DFM */
        return;
    }
    cc -= hlen;
    icp = (struct icmp *)(buf + hlen);
    if ( (!(pingflags & QUIET)) && icp->icmp_type != ICMP_ECHOREPLY )  {
        //printf("%d bytes from %s: icmp_type=%d (%s) icmp_code=%d\n",
        //cc, inet_ntoa(ntohl(from->sin_addr)),
        //icp->icmp_type, pr_type(icp->icmp_type), icp->icmp_code);/*DFM*/
        if (pingflags & VERBOSE) {
            //for( i=0; i<12; i++)
                //printf("x%2.2x: x%8.8x\n", i*sizeof(long),*lp++);
        }
        return;
    }
    if ( icp->icmp_id != ident )
        return;

    if (timing) {
        tp = (struct timeval *)&icp->icmp_data[0];
        tvsub( &tv, tp );
        triptime = tv.tv_sec*1000+(tv.tv_usec/1000);
        tsum += triptime;
        if ( triptime < tmin )
            tmin = triptime;
        if ( triptime > tmax )
            tmax = triptime;
    }

    if (!(pingflags & QUIET)) {
        if (pingflags != FLOOD) {
            //printf("%d bytes from %s: icmp_seq=%d", cc,inet_ntoa(from->sin_addr),icp->icmp_seq );    /* DFM */
            //strncpy(p_addr,"xurenjun\n",9);
            //if (timing)
            //printf(" time=%d ms\n", triptime );
            //else
            //putchar('\n');
        } else {
            putchar('\b');
            fflush(stdout);
        }
    }
    nreceived++;
}


in_cksum(addr, len)
u_short *addr;
int len;
{
    register int nleft = len;
    register u_short *w = addr;
    register u_short answer;
    register int sum = 0;

    while ( nleft > 1 )  {
        sum += *w++;
        nleft -= 2;
    }

    if ( nleft == 1 ) {
        u_short    u = 0;

        *(u_char *)(&u) = *(u_char *)w ;
        sum += u;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}

tvsub( out, in )
register struct timeval *out, *in;
{
    if ( (out->tv_usec -= in->tv_usec) < 0 )   {
        out->tv_sec--;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

void finish(int sig)
{
    putchar('\n');
    fflush(stdout);
    //printf("\n----%s PING Statistics----\n", hostname );
    //printf("%d packets transmitted, ", ntransmitted );
    //printf("%d packets received, ", nreceived );
    if (ntransmitted)
        if ( nreceived > ntransmitted)
            printf("-- somebody's printing up packets!");
        else
            printf("%d%% packet loss",
              (int) (((ntransmitted-nreceived)*100) /
              ntransmitted));
    printf("\n");
    if (nreceived && timing)
        printf("round-trip (ms)  min/avg/max = %d/%d/%d\n",
        tmin,
        tsum / nreceived,
        tmax );
    fflush(stdout);
    exit(0);
}
