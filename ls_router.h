#ifndef LS_ROUTER_H
#define LS_ROUTER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define MAXDATASIZE 256
#define MAXDISTANCE 2048

#define LS_PACKET_SIZE 13
#define PAIR_SIZE 5

#define FREQUENCY 500

//int dist[MAXDATASIZE];
int next_hop[MAXDATASIZE];
int prev[MAXDATASIZE];
bool my_neighbor[MAXDATASIZE];
int my_neighbor_count;

int globalMyID;

//global node address table. Moved from main.c.
struct sockaddr_in globalNodeAddrs[256];

int ** imported_cost_graph;
int ** real_cost_graph;


struct pair {
	uint8_t node;
	uint32_t cost;
};

struct ls_packet {
	uint8_t originating_node;
	uint32_t ttl;
	uint32_t seq_num;
	uint32_t num_pairs;
	struct pair * pairs;
};

int handle_cost_files(const char * cost_file, struct ls_packet * buf, int globalMyID);
int open_logfile(char* file);
void calculate_shortest_path(int ** graph, int source);


#endif