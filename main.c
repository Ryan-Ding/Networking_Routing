#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "ls_router.h"


void listenForNeighbors();
void* announceToNeighbors(void* unusedParam);



//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
struct timeval globalLastHeartbeat[256];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
//struct sockaddr_in globalNodeAddrs[256];

struct ls_packet * lsp;

char test_log_file[50];
 
int main(int argc, char** argv)
{
	int i, j;
	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s mynodeid initialcostsfile logfile\n\n", argv[0]);
		exit(1);
	}

	if(open_logfile(argv[3])== -1)
	{
		printf("Not a valid output file");
		exit(1);
	}




	imported_cost_graph = malloc( MAXDATASIZE * sizeof(int*));
	real_cost_graph = malloc( MAXDATASIZE * sizeof(int*));
	for(i=0; i<MAXDATASIZE; i++)
	{
		imported_cost_graph[i]= malloc( MAXDATASIZE * sizeof(int));
		real_cost_graph[i]= malloc( MAXDATASIZE * sizeof(int));
	}

	for(i = 0; i < MAXDATASIZE; i++)
	{
		my_neighbor[i] = false;
		prev[i] = next_hop[i] = -1;
		for(j = 0; j < MAXDATASIZE; j++)
		{
			imported_cost_graph[i][j] = 1;
			real_cost_graph[i][j] = MAXDISTANCE;

			if(i == j)
				imported_cost_graph[i][j] = real_cost_graph[i][j] = 0;
		}
	}	
	
	//initialization: get this process's node ID, record what time it is, 
	//and set up our sockaddr_in's for sending to the other nodes.
	globalMyID = atoi(argv[1]);

	// sprintf(test_log_file, "testlog/testlog%d.txt", globalMyID);

	// if(open_test_logfile(test_log_file)== -1)
	// {
	// 	printf("Not a valid output file");
	// 	exit(1);
	// }


	//RYAN: I believe this is to record IP address for each potential node.
	for(i=0;i<256;i++)
	{
		gettimeofday(&globalLastHeartbeat[i], 0);
		
		char tempaddr[100];
		sprintf(tempaddr, "10.1.1.%d", i);
		memset(&globalNodeAddrs[i], 0, sizeof(globalNodeAddrs[i]));
		globalNodeAddrs[i].sin_family = AF_INET;
		globalNodeAddrs[i].sin_port = htons(7777);
		inet_pton(AF_INET, tempaddr, &globalNodeAddrs[i].sin_addr);
	}
	
	
	//TODO: read and parse initial costs file. default to cost 1 if no entry for a node. file may be empty.
	//DONE
	
	lsp = malloc (MAXDATASIZE * sizeof(struct ls_packet));
	for (i = 0; i<MAXDATASIZE; i++)
	{
		lsp[i].pairs = malloc( MAXDATASIZE * sizeof(struct pair));
	}

	initialize_my_lsp(lsp);

	int result;
	result = handle_cost_files (argv[2], lsp, globalMyID);
	
	//socket() and bind() our socket. We will do all sendto()ing and recvfrom()ing on this one.
	if((globalSocketUDP=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}
	char myAddr[100];
	struct sockaddr_in bindAddr;
	sprintf(myAddr, "10.1.1.%d", globalMyID);	
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(7777);
	inet_pton(AF_INET, myAddr, &bindAddr.sin_addr);
	if(bind(globalSocketUDP, (struct sockaddr*)&bindAddr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind");
		close(globalSocketUDP);
		exit(1);
	}
	
	
	//start threads... feel free to add your own, and to remove the provided ones.
	pthread_t announcerThread;
	pthread_create(&announcerThread, 0, announceToNeighbors, (void*)0);
	
	
	
	
	//good luck, have fun!
	listenForNeighbors(lsp);
	
	
	
}
