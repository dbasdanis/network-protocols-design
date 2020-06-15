# include <stdio.h>
# include <stdlib.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <netdb.h>
#include <netinet/sctp.h>
# include <string.h>
# include <unistd.h>
# include <fcntl.h>
# include <errno.h>

/*
 * (c) 2011 dermesser
 * This piece of software is licensed with GNU GPL 3.
 *
 */

const char* help_string = "Usage: simple-http [-h] [-4|-6] [-p PORT] [-o OUTPUT_FILE] SERVER FILE\n";

void errExit(const char* str, char p)
{
	if ( p != 0 )
	{
		perror(str);
	} else
	{
		fprintf(stderr,str);
	}
	exit(1);
}

int main (int argc, char** argv)
{
	struct addrinfo *result, hints;
	struct sctp_status status;
    	struct sctp_initmsg initmsg;
    	struct sctp_event_subscribe events;
    	struct sctp_paddrparams heartbeat;
   	struct sctp_rtoinfo rtoinfo;
	
	int srvfd, rwerr = 42, outfile, ai_family = AF_INET, flags = 0;
	char *request, buf[16], port[6],c;

	memset(port,0,6);
	memset(&initmsg,    0,   sizeof(struct sctp_initmsg));
	memset(&events,     1,   sizeof(struct sctp_event_subscribe));
    	memset(&status,     0,   sizeof(struct sctp_status));
    	memset(&heartbeat,  0,   sizeof(struct sctp_paddrparams));
    	memset(&rtoinfo,    0,   sizeof(struct sctp_rtoinfo));

	if ( argc < 3 )
		errExit(help_string,0);
	
	strncpy(port,"80",2);

	while ( (c = getopt(argc,argv,"p:ho:46")) >= 0 )
	{
		switch (c)
		{
			case 'h' :
				errExit(help_string,0);
			case 'p' :
				strncpy(port,optarg,5);
				break;
			case 'o' :
				outfile = open(optarg,O_WRONLY|O_CREAT,0644);
				close(1);
				dup2(outfile,1);
				break;
			case '4' :
				ai_family = AF_INET;
				break;
			case '6' :
				ai_family = AF_INET6;
				break;
		}
	}

	memset(&hints,0,sizeof(struct addrinfo));

	hints.ai_family = ai_family;
	hints.ai_socktype = SOCK_STREAM;

	initmsg.sinit_num_ostreams = 2;
        initmsg.sinit_max_instreams = 2;
        initmsg.sinit_max_attempts = 1;

    	heartbeat.spp_flags = SPP_HB_ENABLE;
    	heartbeat.spp_hbinterval = 5000;
    	heartbeat.spp_pathmaxrxt = 1;

    	rtoinfo.srto_max = 2000;

	if ( 0 != getaddrinfo(argv[optind],port,&hints,&result))
		errExit("getaddrinfo",1);

	// Create socket after retrieving the inet protocol to use (getaddrinfo)
	srvfd = socket(result->ai_family,SOCK_STREAM,IPPROTO_SCTP);

	if ( srvfd < 0 )
		errExit("socket()",1);
	
	setsockopt(srvfd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS , &heartbeat, sizeof(heartbeat));
	setsockopt(srvfd, SOL_SCTP, SCTP_RTOINFO , &rtoinfo, sizeof(rtoinfo));
	setsockopt(srvfd, SOL_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg));
	setsockopt(srvfd, SOL_SCTP, SCTP_EVENTS, (void *)&events, sizeof(events));
	

	if ( connect(srvfd,result->ai_addr,result->ai_addrlen) == -1)
		errExit("connect",1);
	
	
	struct sockaddr_in *paddrs[5];
	int addr_count = sctp_getpaddrs(srvfd, 0, (struct sockaddr**)paddrs);
        printf("\nPeer addresses: %d\n", addr_count);

       /*Print Out Addresses*/
        for(int i = 0; i < addr_count; i++)
       		printf("Address %d: %s:%d\n", i +1, inet_ntoa((*paddrs)[i].sin_addr), (*paddrs)[i].sin_port);	
	// Now we have an established connection.
	
	// XXX: Change the length if request string is modified!!!
	request = calloc(53+strlen(argv[optind+1])+strlen(argv[optind]),1);

	sprintf(request,"GET %s HTTP/1.1\nHost: %s\nUser-agent: simple-http client\n\n",argv[optind+1],argv[optind]);


//	write(srvfd,request,strlen(request));
	//int q=0;
	//while(q<10){
	send(srvfd,request,strlen(request),0);
	//sctp_sendmsg(srvfd,(void *) request,strlen(request),NULL,0,0,0,0,0,0);
//	q++;
	//	printf("q:%d\n",q);
//	}
	shutdown(srvfd,SHUT_WR);

	while ( rwerr > 0 )
	{
		rwerr = read(srvfd,buf,16);
		//rwerr = sctp_recvmsg(srvfd,buf,16,NULL,0,NULL,&flags);
		write(1,buf,strlen(buf));
	}
	
	close(srvfd);

	return 0;

}
