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

extern int globalMyID;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
extern struct timeval globalLastHeartbeat[256];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
extern int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
extern struct sockaddr_in globalNodeAddrs[256];

extern struct ls_packet * lsp;


//Yes, this is terrible. It's also terrible that, in Linux, a socket
//can't receive broadcast packets unless it's bound to INADDR_ANY,
//which we can't do in this assignment.
void hackyBroadcast(const char* buf, int length)
{
	int i;
	for(i=0;i<256;i++)
		if(i != globalMyID) //(although with a real broadcast you would also get the packet yourself)
			sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
}

void* announceToNeighbors(void* unusedParam)
{
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 800 * 1000 * 1000; //1000 ms
	int i;

	char local_lsp_buf[3 + LS_PACKET_SIZE + lsp[globalMyID].num_pairs * PAIR_SIZE];
	struct ls_packet send_ls;
	while(1)
	{
		//hackyBroadcast("HEREIAM", 7);

		lsp[globalMyID].seq_num++;
		//printf("sending seq number %d\n", lsp[globalMyID].seq_num);
		send_ls = lsp[globalMyID];
		send_ls.seq_num = htonl(send_ls.seq_num);
		send_ls.num_pairs = htonl(send_ls.num_pairs);
		memcpy(local_lsp_buf, "lsp", 3);
		memcpy(local_lsp_buf + 3, &send_ls.originating_node, 1);
		memcpy(local_lsp_buf + 4, &send_ls.ttl, 4);
		memcpy(local_lsp_buf + 8, &send_ls.seq_num, 4);
		memcpy(local_lsp_buf + 12, &send_ls.num_pairs, 4);
		for (i = 0; i < lsp[globalMyID].num_pairs; ++i)
		{
			//printf("sending node %d, cost %d\n", send_ls.pairs[i].node , send_ls.pairs[i].cost);
			//send_ls.pairs[i].cost = htonl(send_ls.pairs[i].cost);
			memcpy(local_lsp_buf + 16 + PAIR_SIZE*i, &send_ls.pairs[i].node, 1);
			memcpy(local_lsp_buf + 17 + PAIR_SIZE*i, &send_ls.pairs[i].cost, 4);
		}
		//print_hex(local_lsp_buf);
		//printf("ls packet size is %d\n", (int)sizeof(struct ls_packet));
		//printf("string length is %d\n", (int)strlen(local_lsp_buf));
		broadcast_lsp_message(globalSocketUDP, local_lsp_buf, 3 + LS_PACKET_SIZE + lsp[globalMyID].num_pairs * PAIR_SIZE, globalMyID);
		
		nanosleep(&sleepFor, 0);
	}
}

void broadcast_local_lsp_once()
{
	char local_lsp_buf[3 + LS_PACKET_SIZE + lsp[globalMyID].num_pairs * PAIR_SIZE];
	struct ls_packet send_ls;
	int i;

	    lsp[globalMyID].seq_num++;
		//printf("sending seq number %d\n", lsp[globalMyID].seq_num);
		send_ls = lsp[globalMyID];
		send_ls.seq_num = htonl(send_ls.seq_num);
		send_ls.num_pairs = htonl(send_ls.num_pairs);
		memcpy(local_lsp_buf, "lsp", 3);
		memcpy(local_lsp_buf + 3, &send_ls.originating_node, 1);
		memcpy(local_lsp_buf + 4, &send_ls.ttl, 4);
		memcpy(local_lsp_buf + 8, &send_ls.seq_num, 4);
		memcpy(local_lsp_buf + 12, &send_ls.num_pairs, 4);
		for (i = 0; i < lsp[globalMyID].num_pairs; ++i)
		{
			
			memcpy(local_lsp_buf + 16 + PAIR_SIZE*i, &send_ls.pairs[i].node, 1);
			memcpy(local_lsp_buf + 17 + PAIR_SIZE*i, &send_ls.pairs[i].cost, 4);
		}
		broadcast_lsp_message(globalSocketUDP, local_lsp_buf, 3 + LS_PACKET_SIZE + lsp[globalMyID].num_pairs * PAIR_SIZE, globalMyID);
}

void listenForNeighbors(struct ls_packet * lsp)
{
	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	unsigned char recvBuf[1000];

	int bytesRecvd;
	while(1)
	{
		//Probably want to calculate the graph constantly
		//calculate_shortest_path(real_cost_graph, globalMyID);
		//update_my_lsp(lsp);
		//print_graph(real_cost_graph);

		theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvBuf, 1000 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
		recvBuf[bytesRecvd] = '\0';
		
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);
		
		short int heardFrom = -1;
		if(strstr(fromAddr, "10.1.1."))
		{
			heardFrom = atoi(
					strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
			
			//TODO: this node can consider heardFrom to be directly connected to it; do any such logic now.
			my_neighbor[heardFrom] = true;
			if(real_cost_graph[globalMyID][heardFrom] >= MAXDISTANCE ||
				real_cost_graph[globalMyID][heardFrom] != imported_cost_graph[globalMyID][heardFrom])
			{
				//printf("global id = %d, heardfrom = %d\n",globalMyID, heardFrom);
				//printf("Imported cost is %d\n", imported_cost_graph[globalMyID][heardFrom]);
				real_cost_graph[globalMyID][heardFrom] = imported_cost_graph[globalMyID][heardFrom];
				calculate_shortest_path(real_cost_graph, globalMyID);
				
				//update only globalmyID lsp.
				update_my_lsp(lsp);
				broadcast_local_lsp_once();
			}
			//record that we heard from heardFrom just now
			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}

		struct timeval now;
		gettimeofday(&now, 0);
		int i;
		for (i = 0; i < MAXDATASIZE; i++)
		{
			if(my_neighbor[i])
			{
			// printf("Time in microseconds: %ld microseconds\n",
			// 	(now.tv_sec - globalLastHeartbeat[i].tv_sec)*1000000L
   //         + now.tv_usec - globalLastHeartbeat[i].tv_usec);

				if((now.tv_sec - globalLastHeartbeat[i].tv_sec)*1000000L
           + now.tv_usec - globalLastHeartbeat[i].tv_usec > 800 * 4000) //more than 4 broadcast time
				{
					//cut connection
					//printf("MY NODE %d to NODE %d CONNECTION IS CUT!!!!!!\n", globalMyID, i);
					my_neighbor[i] = false;
					update_my_lsp(lsp);
					broadcast_local_lsp_once();
					real_cost_graph[globalMyID][i] = MAXDISTANCE;
					calculate_shortest_path(real_cost_graph, globalMyID);
				}
			}
		}
		
		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if(!strncmp(recvBuf, "send", 4))
		{
			//TODO send the requested message to the requested destination node
			// ...
			printf("node %d heard a send packet\n", globalMyID);
			int8_t dest1 = recvBuf[4];
			int16_t dest2 = (dest1 << 8 ) | ((uint8_t)recvBuf[5]);
			if(dest2 <= 255 && dest2 >= 0)
			{
				if(globalMyID == dest2)
				{
					printf("Destination node reached %d\n", dest2);
					log_receiving_packet(&recvBuf[6]);
				}
				else 
				{
					calculate_shortest_path(real_cost_graph, globalMyID);
					printf("Received message %s\n", &recvBuf[6]);
					printf("preparing to send to final dest %d through node next_hop %d\n", dest2, next_hop[dest2]);
					if(next_hop[dest2] != -1)
						send_packet(globalSocketUDP, next_hop[dest2], dest2, recvBuf, bytesRecvd);
					else
						log_unreachable_event(dest2);
				}
			}
			else log_unreachable_event(dest2);

		}

		else if(!strncmp(recvBuf, "fwrd", 4))
		{
			//TODO send the requested message to the requested destination node
			// ...
			printf("node %d heard a forward packet\n", globalMyID);
			int8_t dest1 = recvBuf[4];
			int16_t dest2 = (dest1 << 8 ) | ((uint8_t)recvBuf[5]);
			if(dest2 <= 255 && dest2 >= 0)
			{
				if(globalMyID == dest2)
				{
					printf("Destination node reached %d\n", dest2);
					log_receiving_packet(&recvBuf[6]);
				}
				else 
				{
					calculate_shortest_path(real_cost_graph, globalMyID);
					printf("Received message %s\n", &recvBuf[6]);
					printf("preparing to send to final dest %d through node next_hop %d\n", dest2, next_hop[dest2]);
					if(next_hop[dest2] != -1)
						forward_send_packet(globalSocketUDP, next_hop[dest2], dest2, recvBuf, bytesRecvd);
					else 
						log_unreachable_event(dest2);
				}
			}
			else log_unreachable_event(dest2);

		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if(!strncmp(recvBuf, "cost", 4))
		{
			//TODO record the cost change (remember, the link might currently be down! in that case,
			//this is the new cost you should treat it as having once it comes back up.)
			// ...
			uint8_t temp= recvBuf[4];
			uint16_t NeighborID = (temp << 8 ) | ((uint8_t)recvBuf[5]);
			uint32_t no_NewCost, NewCost;
			memcpy(&no_NewCost, &recvBuf[6], 4);
			NewCost = ntohl(no_NewCost);
			if(NeighborID <= 255 && NeighborID >= 0)
			{
				if(globalMyID != NeighborID)
				{
					printf("change neighbor %d cost to %d\n", NeighborID, NewCost );
					imported_cost_graph[globalMyID][NeighborID] = NewCost;
					if(my_neighbor[NeighborID])
					{
						real_cost_graph[globalMyID][NeighborID] = NewCost;
						update_my_lsp(lsp);
						broadcast_local_lsp_once();
						calculate_shortest_path(real_cost_graph, globalMyID);
					}
				}
			}
		}

		// this is to handle lsp flooding information.
		else if(!strncmp(recvBuf, "lsp", 3))
		{
			
			handle_lsp_input_enhanced(globalSocketUDP, recvBuf, bytesRecvd, heardFrom, lsp);
			//print_graph(real_cost_graph);
			//print_neighbor_list();

		}
		
		//TODO now check for the various types of packets you use in your own protocol
		//else if(!strncmp(recvBuf, "your other message types", ))
		// ... 
	}
	//(should never reach here)
	close(globalSocketUDP);
}

