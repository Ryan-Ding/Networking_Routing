#include "ls_router.h"

char log_buffer[MAXDATASIZE];
FILE * fd_cost;
FILE * fd_log;
FILE * test_fd_log;

void print_hex(const char * s)
{
	while(*s)
	{
		printf("%02x\n", (unsigned int) *s++ );
	}
}

int handle_cost_files( const char * cost_file, struct ls_packet * buf, int globalMyID){
	char * temp_buf = NULL;
	int i;
	size_t buffer_size = MAXDATASIZE;
	FILE * fd_cost;
	fd_cost = fopen( cost_file, "r");
	if(!fd_cost)
	{
		return -1;
	}

	while (getline(&temp_buf, &buffer_size, fd_cost) != -1){
		imported_cost_graph[globalMyID][atoi(temp_buf)] = atoi(strchr(temp_buf, ' ')+1);
		//printf("%d ",atoi(temp_buf));
        //printf("%d\n",atoi(strchr(temp_buf, ' ')+1));
	}

	fclose(fd_cost);
	return 0;
}


void broadcast_lsp_message(int globalSocketUDP, char * buf, int length, int heardFrom)
{
	int i;
	for(i=0;i<256;i++)
		if(i != globalMyID && i != heardFrom) 
			sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
}

int check_if_lsp_updated(struct ls_packet *old_lsp, struct ls_packet *new_lsp)
{
	int i = 0;
	if(old_lsp -> num_pairs != new_lsp -> num_pairs)
		return 1;
	else
	{
		for(i = 0; i < new_lsp->num_pairs; i++)
		{
			if(old_lsp -> pairs[i].node != new_lsp -> pairs[i].node)
				return 1;
			else
			{
				if(old_lsp -> pairs[i].cost != new_lsp -> pairs[i].cost)
					return 1;
			}
		}
		return 0;
	}
}

void update_local_lsp(struct ls_packet *lsp, struct ls_packet *new_lsp, uint8_t originating_node)
{
	int i;
	lsp[originating_node].seq_num = new_lsp -> seq_num;
	lsp[originating_node].ttl = new_lsp -> ttl;
	lsp[originating_node].num_pairs = new_lsp -> num_pairs;

	for (i = 0; i < MAXDATASIZE; ++i)
		{
			real_cost_graph[originating_node][i] = MAXDISTANCE;
		}

	for(i = 0; i < new_lsp -> num_pairs; i++)
	{
		lsp[originating_node].pairs[i].node = new_lsp->pairs[i].node;
		lsp[originating_node].pairs[i].cost = new_lsp->pairs[i].cost;

		real_cost_graph[originating_node][lsp[originating_node].pairs[i].node]
			= lsp[originating_node].pairs[i].cost;
	}
}

void handle_lsp_input_enhanced(int globalSocketUDP, char * input_buf, int bytesRecvd, int heardFrom, struct ls_packet * lsp)
{
	int i;
	uint8_t originating_node;
	uint32_t ttl, seq_num, num_pairs;
	struct ls_packet my_local_packet;

	memcpy(&originating_node, input_buf + 3, sizeof(uint8_t));
	memcpy(&ttl, input_buf + 3 + sizeof(uint8_t), sizeof(uint32_t));
	ttl = ntohl(ttl);
	memcpy(&seq_num, input_buf + 3 + sizeof(uint8_t) + sizeof(uint32_t), sizeof(uint32_t));
	seq_num = ntohl(seq_num);
	memcpy(&num_pairs, input_buf + 3 + sizeof(uint8_t) + 2* sizeof(uint32_t), sizeof(uint32_t));
	num_pairs = ntohl(num_pairs);

	//reserve space for pairs
	my_local_packet.pairs = malloc( num_pairs * sizeof(struct pair));

	if(seq_num > lsp[originating_node].seq_num)
	{
		my_local_packet.seq_num = seq_num;
		my_local_packet.ttl = ttl;
		my_local_packet.num_pairs = num_pairs;
		//printf("originitting node = %d; num_pair = %d; seq_num = %d\n", originating_node, num_pairs, seq_num);
		

		for(i = 0; i < num_pairs; i++)
		{
			memcpy(&my_local_packet.pairs[i].node, 
				input_buf + 16 + i * PAIR_SIZE, 1);
			memcpy(&my_local_packet.pairs[i].cost, 
				input_buf + 17 + i * PAIR_SIZE , 4);

		}
		if (check_if_lsp_updated(&lsp[originating_node], &my_local_packet))
		{
			//printf("updated lsp\n");
			update_local_lsp(lsp, &my_local_packet, originating_node);
			calculate_shortest_path(real_cost_graph, globalMyID);
			broadcast_lsp_message(globalSocketUDP, input_buf, bytesRecvd, heardFrom);
		}
		// else
		// 	printf("new lsp same as old one\n");
	}
}

void handle_lsp_input(int globalSocketUDP, char * input_buf, int bytesRecvd, int heardFrom, struct ls_packet * lsp)
{
	int i;
	uint8_t originating_node;
	uint32_t ttl, seq_num, num_pairs;

	memcpy(&originating_node, input_buf + 3, sizeof(uint8_t));
	memcpy(&ttl, input_buf + 3 + sizeof(uint8_t), sizeof(uint32_t));
	ttl = ntohl(ttl);
	memcpy(&seq_num, input_buf + 3 + sizeof(uint8_t) + sizeof(uint32_t), sizeof(uint32_t));
	seq_num = ntohl(seq_num);
	memcpy(&num_pairs, input_buf + 3 + sizeof(uint8_t) + 2* sizeof(uint32_t), sizeof(uint32_t));
	num_pairs = ntohl(num_pairs);

	if(seq_num > lsp[originating_node].seq_num)
	{
		lsp[originating_node].seq_num = seq_num;
		lsp[originating_node].ttl = ttl;
		lsp[originating_node].num_pairs = num_pairs;
		//printf("originitting node = %d; num_pair = %d; seq_num = %d\n", originating_node, num_pairs, seq_num);
		
		for (i = 0; i < MAXDATASIZE; ++i)
		{
			real_cost_graph[originating_node][i] = MAXDISTANCE;
		}

		for(i = 0; i < num_pairs; i++)
		{
			memcpy(&lsp[originating_node].pairs[i].node, 
				input_buf + 16 + i * PAIR_SIZE, 1);
			memcpy(&lsp[originating_node].pairs[i].cost, 
				input_buf + 17 + i * PAIR_SIZE , 4);

			//lsp[originating_node].pairs[i].cost = ntohl(lsp[originating_node].pairs[i].cost);
			
			//printf("PAIR %d: node = %d, cost = %d\n", i, lsp[originating_node].pairs[i].node, lsp[originating_node].pairs[i].cost );

			real_cost_graph[originating_node][lsp[originating_node].pairs[i].node]
			= lsp[originating_node].pairs[i].cost;

		}
		broadcast_lsp_message(globalSocketUDP, input_buf, bytesRecvd, heardFrom);
	}
}

int open_logfile(char * file)
{
	printf("logfile opened %s\n", file);
	fd_log = fopen(file, "w");
	if(!fd_log) 
	{
		return 1;
	}
	return 0;
}

int open_test_logfile(char * file)
{
	printf("testlogfile opened %s\n", file);
	test_fd_log = fopen(file, "w");
	if(!test_fd_log) 
	{
		return 1;
	}
	return 0;
}


void log_sending_packet(int dest, int nexthop, char *data)
{
  sprintf( log_buffer, "sending packet dest %d nexthop %d message %s\n", dest, nexthop, data);
  fwrite( log_buffer, 1, strlen(log_buffer), fd_log);
  fflush(fd_log);
}

void log_forwarding_packet(int dest, int nexthop, char *data)
{
  sprintf( log_buffer, "forward packet dest %d nexthop %d message %s\n", dest, nexthop, data);
  fwrite( log_buffer, 1, strlen(log_buffer), fd_log);
  fflush(fd_log);
}

void log_receiving_packet( char *data)
{
  sprintf( log_buffer, "receive packet message %s\n", data);
  fwrite( log_buffer, 1, strlen(log_buffer), fd_log);
  fflush(fd_log);
}

void log_unreachable_event(int dest)
{
  sprintf(log_buffer, "unreachable dest %d\n", dest);
  fwrite( log_buffer, 1, strlen(log_buffer), fd_log);
  fflush(fd_log);
}


void send_packet(int globalSocketUDP, int nextHop, int dest, char *buf, int length)
{

	memcpy(buf, "fwrd", 4);

	printf("sending a packet\n");
	if(sendto(globalSocketUDP, buf, length, 0, (struct sockaddr*)&globalNodeAddrs[nextHop], sizeof(globalNodeAddrs[nextHop])) < 0)
      perror("sendto()");

  	log_sending_packet(dest, nextHop, &buf[6]);

}

void forward_send_packet(int globalSocketUDP, int nextHop, int dest, char * buf, int length)
{
	printf("forwarding a send packet to %d\n", nextHop);
	if(sendto(globalSocketUDP, buf, length, 0, (struct sockaddr*)&globalNodeAddrs[nextHop], sizeof(globalNodeAddrs[nextHop])) < 0)
      perror("sendto()");
  	log_forwarding_packet(dest, nextHop, &buf[6]);
}

void initialize_my_lsp(struct ls_packet * lsp)
{
	int i;
	for(i = 0; i < MAXDATASIZE; i++)
	{
		lsp[i].originating_node = i;
		lsp[i].ttl = MAXDATASIZE;
		lsp[i].seq_num = 0;
		lsp[i].num_pairs = 0;
	}
}

void update_my_lsp(struct ls_packet *  lsp)
{
	int i, counter;
	counter = 0;
	for(i = 0; i< MAXDATASIZE; i++)
	{
		if(my_neighbor[i])
		{
			lsp[globalMyID].pairs[counter].node = i;
			lsp[globalMyID].pairs[counter].cost = real_cost_graph[globalMyID][i];
			//printf("neighbor %d; cost %d\n", i, lsp[globalMyID].pairs[counter].cost);
			counter ++;
		}
	}
	lsp[globalMyID].num_pairs = counter;
}


int minDistance (int dist[], bool sptSet[])
{
	int min = MAXDISTANCE, min_index;

	int v;
	for(v = MAXDATASIZE - 1; v >= 0; v--)
	{
		if(sptSet[v] == false && dist[v] <= min)
			min = dist[v], min_index = v;
	}

	return min_index;
}

void calculate_shortest_path(int ** graph, int source){
	int dist[MAXDATASIZE];
	bool SptSet[MAXDATASIZE];
	int i,j,u;
	//printf("entered dijkstra\n");

	for(i = 0; i< MAXDATASIZE; i++)
	{
		dist[i] = MAXDISTANCE;
		SptSet[i] = false;
		prev[i] = -1;
		next_hop[i] = -1; 
	}

	dist[globalMyID] = 0;

	for( i = 0; i< MAXDATASIZE; i++)
	{
		u = minDistance(dist, SptSet);
		SptSet[u] = true;

		for(j = 0; j < MAXDATASIZE; j++)
		{
			if(!SptSet[j] && graph[u][j] < MAXDISTANCE && dist[u] < MAXDISTANCE 
				&& dist[u]+graph[u][j] <= dist[j])
			{
				if(dist[u]+graph[u][j] == dist[j])
				{
					if(prev[j] != -1 && prev[j] > u)
					{
						dist[j] = dist[u] + graph[u][j];
						prev[j] = u;
					}
				}
				else
				{
					dist[j] = dist[u] + graph[u][j];
					prev[j] = u;
				}
			}
		}
	}

	for( i = 0; i < MAXDATASIZE; i++)
	{
		j = i;
		while(prev[j] != -1)
		{
			if(prev[j] == globalMyID)
			{
				next_hop[i] = j;
			}
			else
			{
				next_hop[i] = prev[j];
			}
			j = prev[j];
		}
	}
	//print_solution(dist);
	//printf("left dijkstra\n");
}

int print_solution( int dist[])
{
	int i;
	printf("print distances here:\n");
	for (i = 0; i< 10; i++)
	{
		printf("%d tt %d\n", i, dist[i] );
	}
}

void print_graph(int ** graph, int source){
	int i,j;
	for(i = 0; i < 20; i++)
	{
		for(j = 0; j < 20; j++)
		{
			printf("edge %d, %d  cost %d\n", i, j, graph[i][j]);
		}
	}
}

void print_neighbor_list()
{
	int i;
	printf("Neighborlist: \n");
	for (i = 0; i < 10; ++i)
	{
		if(my_neighbor[i])
			printf("%d\n", i);
	}
	printf("\n");
}