#include "mospf_daemon.h"
#include "mospf_proto.h"
#include "mospf_nbr.h"
#include "mospf_database.h"

#include "ip.h"

#include "list.h"
#include "log.h"
#include "packet.h"
#include "rtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

extern ustack_t *instance;

pthread_mutex_t mospf_lock;
pthread_mutex_t mospf_database_lock;

void mospf_init()
{
	pthread_mutex_init(&mospf_lock, NULL);
	pthread_mutex_init(&mospf_database_lock, NULL);

	instance->area_id = 0;
	// get the ip address of the first interface
	iface_info_t *iface = list_entry(instance->iface_list.next, iface_info_t, list);
	instance->router_id = iface->ip;
	instance->sequence_num = 0;
	instance->lsuint = MOSPF_DEFAULT_LSUINT;

	iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		iface->helloint = MOSPF_DEFAULT_HELLOINT;
		init_list_head(&iface->nbr_list);
	}

	init_mospf_db();
}

void *sending_mospf_hello_thread(void *param);
void *sending_mospf_lsu_thread(void *param);
void sending_mospf_lsu();
void *checking_nbr_thread(void *param);
void *checking_database_thread(void *param);
void *generating_rtable_thread(void *param);

void mospf_run()
{
	pthread_t hello, lsu, nbr, db, rt;
	pthread_create(&hello, NULL, sending_mospf_hello_thread, NULL);
	pthread_create(&lsu, NULL, sending_mospf_lsu_thread, NULL);
	pthread_create(&nbr, NULL, checking_nbr_thread, NULL);
	pthread_create(&db, NULL, checking_database_thread, NULL);
	pthread_create(&rt, NULL, generating_rtable_thread, NULL);
}

void *sending_mospf_hello_thread(void *param)
{
	fprintf(stdout, "TODO: send mOSPF Hello message periodically.\n");
	while (1){
		sleep(MOSPF_DEFAULT_HELLOINT);
		pthread_mutex_lock(&mospf_lock);
		iface_info_t * iface = NULL;
		list_for_each_entry (iface, &instance->iface_list, list) {
			int len = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE;
			char * packet = (char*)malloc(len);
		
			struct ether_header *eh = (struct ether_header *)packet;
			eh->ether_type = htons(ETH_P_IP);
			memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
			u8 dhost[ETH_ALEN] = {0x01,0x00,0x5e,0x00,0x00,0x05};
			memcpy(eh->ether_dhost, dhost, ETH_ALEN);

			struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
			u32 dst_ip = MOSPF_ALLSPFRouters;
			ip_init_hdr(ip_hdr,
						iface->ip,
						dst_ip,
						len - ETHER_HDR_SIZE,
						IPPROTO_MOSPF);

			struct mospf_hdr * mospf_header = (struct mospf_hdr *)((char *)ip_hdr + IP_BASE_HDR_SIZE);
			mospf_init_hdr(mospf_header,
						   MOSPF_TYPE_HELLO,
						   len - ETHER_HDR_SIZE - IP_BASE_HDR_SIZE,
						   instance->router_id,
						   0);

			struct mospf_hello * hello = (struct mospf_hello *)((char *)mospf_header + MOSPF_HDR_SIZE);
			mospf_init_hello(hello, iface->mask);

			mospf_header->checksum = mospf_checksum(mospf_header);
			iface_send_packet(iface, packet, len);
		}
		pthread_mutex_unlock(&mospf_lock);
	}
	return NULL;
}

void *checking_nbr_thread(void *param)
{
	fprintf(stdout, "TODO: neighbor list timeout operation.\n");
	while (1) {
		sleep(1);
		mospf_nbr_t * nbr_pos = NULL, *nbr_q = NULL;
		iface_info_t *iface = NULL;
		pthread_mutex_lock(&mospf_lock);
		list_for_each_entry(iface, &instance->iface_list, list) {
			list_for_each_entry_safe(nbr_pos, nbr_q, &iface->nbr_list, list) {
				nbr_pos->alive++;
				if (nbr_pos->alive > 3 * iface->helloint) {
					fprintf(stdout, "TODO: free a neighbor node.\n");
					list_delete_entry(&nbr_pos->list);
					free(nbr_pos);
					iface->num_nbr--;
					sending_mospf_lsu();
				}
			}
		}
		pthread_mutex_unlock(&mospf_lock);
	}
	
	return NULL;
}

void *checking_database_thread(void *param)
{
	fprintf(stdout, "TODO: link state database timeout operation.\n");
	while (1) {
		sleep(1);
		mospf_db_entry_t * db_pos = NULL, *db_q = NULL;
		iface_info_t *iface = NULL;
		pthread_mutex_lock(&mospf_lock);
		list_for_each_entry_safe(db_pos, db_q, &mospf_db, list) {
			db_pos->alive++;
			if (db_pos->alive > MOSPF_DATABASE_TIMEOUT) {
				list_delete_entry(&db_pos->list);
				free(db_pos);
			}
		}
		pthread_mutex_unlock(&mospf_lock);
	}

	return NULL;
}


void handle_mospf_hello(iface_info_t *iface, const char *packet, int len)
{
	fprintf(stdout, "TODO: handle mOSPF Hello message.\n");
	pthread_mutex_lock(&mospf_lock);
	mospf_nbr_t * nbr_pos = NULL;
	struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
	struct mospf_hdr * mospf_head = (struct mospf_hdr *)((char*)ip_hdr + IP_HDR_SIZE(ip_hdr));
	struct mospf_hello * hello = (struct mospf_hello *)((char*)mospf_head + MOSPF_HDR_SIZE);
	u32 id = ntohl(mospf_head->rid);
	u32 ip = ntohl(ip_hdr->saddr);
	u32 mask = ntohl(hello->mask);
	int isFound = 0; 
	
	list_for_each_entry(nbr_pos, &iface->nbr_list, list) {
		if (nbr_pos->nbr_ip == ip) {
			nbr_pos->alive = 0;
			isFound = 1;
			break;
		}
	}
	if (!isFound) {
		nbr_pos = (mospf_nbr_t *) malloc(sizeof(mospf_nbr_t));
		nbr_pos->alive = 0;
		nbr_pos->nbr_id = id;
		nbr_pos->nbr_ip = ip;
		nbr_pos->nbr_mask = mask;
		//init_list_head(&nbr_pos->list);
		list_add_tail(&(nbr_pos->list), &iface->nbr_list);
		iface->num_nbr++;
	}
	pthread_mutex_unlock(&mospf_lock);
	
}

void sending_mospf_lsu()
{
	iface_info_t * iface = NULL;
	int nbr_num = 0;
	list_for_each_entry (iface, &instance->iface_list, list) { 
		if (iface->num_nbr == 0) {
			nbr_num ++;
		} else {
			nbr_num += iface->num_nbr;
		}	
	}

	struct mospf_lsa * array = (struct mospf_lsa *)malloc(nbr_num * MOSPF_LSA_SIZE);
	mospf_nbr_t * nbr_pos = NULL;

	int index = 0;

	list_for_each_entry (iface, &instance->iface_list, list) {
		if (iface->num_nbr == 0) {
			array[index].mask = htonl(iface->mask);
			array[index].network = htonl(iface->ip & iface->mask);
			array[index].rid = 0;
			index++;
		} else {
			list_for_each_entry (nbr_pos, &iface->nbr_list, list) {
				array[index].mask = htonl(nbr_pos->nbr_mask);
				array[index].network = htonl(nbr_pos->nbr_ip & nbr_pos->nbr_mask);
				array[index].rid = htonl(nbr_pos->nbr_id);
				index++;
			}
		}	
	}

	int len = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_LSU_SIZE + nbr_num * MOSPF_LSA_SIZE;
	
	list_for_each_entry (iface, &instance->iface_list, list) { 
		nbr_pos = NULL;
		list_for_each_entry (nbr_pos, &iface->nbr_list, list) {
			char * packet = (char*)malloc(len);
			struct ether_header *eh = (struct ether_header *)packet;
			eh->ether_type = htons(ETH_P_IP);
			memcpy(eh->ether_shost, iface->mac, ETH_ALEN);

			struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
			u32 dst_ip = nbr_pos->nbr_ip;
			ip_init_hdr(ip_hdr,
						iface->ip,
						dst_ip,
						len - ETHER_HDR_SIZE,
						IPPROTO_MOSPF);

			struct mospf_hdr * mospf_header = (struct mospf_hdr *)((char*)ip_hdr + IP_BASE_HDR_SIZE);
			mospf_init_hdr(mospf_header,
							MOSPF_TYPE_LSU,
							len - ETHER_HDR_SIZE - IP_BASE_HDR_SIZE,
							instance->router_id,
							instance->area_id);

			struct mospf_lsu * lsu = (struct mospf_lsu *)((char*)mospf_header + MOSPF_HDR_SIZE);
			mospf_init_lsu(lsu, nbr_num);
			memcpy((char*)((char*)lsu + MOSPF_LSU_SIZE), array, nbr_num * MOSPF_LSA_SIZE);

			mospf_header->checksum = mospf_checksum(mospf_header);
			ip_send_packet(packet, len);
		}
	}	
	instance->sequence_num++;
	free(array);		
}

void *sending_mospf_lsu_thread(void *param)
{
	fprintf(stdout, "TODO: send mOSPF LSU message periodically.\n");
	while (1){
		sleep(MOSPF_DEFAULT_LSUINT);
		pthread_mutex_lock(&mospf_lock);
		sending_mospf_lsu();
		pthread_mutex_unlock(&mospf_lock);
	}
}

// void *sending_mospf_lsu_thread(void *param)
// {
// 	fprintf(stdout, "TODO: send mOSPF LSU message periodically.\n");
// 	while (1){
// 		sleep(MOSPF_DEFAULT_LSUINT);
// 		pthread_mutex_lock(&mospf_lock);
// 		iface_info_t * iface = NULL;
// 		int nbr_num = 0;
// 		list_for_each_entry (iface, &instance->iface_list, list) { 
// 			if (iface->num_nbr == 0) {
// 				nbr_num ++;
// 			} else {
// 				nbr_num += iface->num_nbr;
// 			}	
// 		}
// 		pthread_mutex_unlock(&mospf_lock);

// 		struct mospf_lsa * array = (struct mospf_lsa *)malloc(nbr_num * MOSPF_LSA_SIZE);
// 		mospf_nbr_t * nbr_pos = NULL;

// 		int index = 0;
// 		pthread_mutex_lock(&mospf_lock);
// 		list_for_each_entry (iface, &instance->iface_list, list) {
// 			if (iface->num_nbr == 0) {
// 				array[index].mask = htonl(iface->mask);
// 				array[index].network = htonl(iface->ip & iface->mask);
// 				array[index].rid = 0;
// 				index++;
// 			} else {
// 				list_for_each_entry (nbr_pos, &iface->nbr_list, list) {
// 					array[index].mask = htonl(nbr_pos->nbr_mask);
// 					array[index].network = htonl(nbr_pos->nbr_ip & nbr_pos->nbr_mask);
// 					array[index].rid = htonl(nbr_pos->nbr_id);
// 					index++;
// 				}
// 			}	
// 		}

// 		int len = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_LSU_SIZE + nbr_num * MOSPF_LSA_SIZE;
		
// 		list_for_each_entry (iface, &instance->iface_list, list) { 
// 			nbr_pos = NULL;
// 			list_for_each_entry (nbr_pos, &iface->nbr_list, list) {
// 				char * packet = (char*)malloc(len);
// 				struct ether_header *eh = (struct ether_header *)packet;
// 				eh->ether_type = htons(ETH_P_IP);
// 				memcpy(eh->ether_shost, iface->mac, ETH_ALEN);

// 				struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
// 				u32 dst_ip = nbr_pos->nbr_ip;
// 				ip_init_hdr(ip_hdr,
// 							iface->ip,
// 							dst_ip,
// 							len - ETHER_HDR_SIZE,
// 							IPPROTO_MOSPF);

// 				struct mospf_hdr * mospf_header = (struct mospf_hdr *)((char*)ip_hdr + IP_BASE_HDR_SIZE);
// 				mospf_init_hdr(mospf_header,
// 								MOSPF_TYPE_LSU,
// 								len - ETHER_HDR_SIZE - IP_BASE_HDR_SIZE,
// 								instance->router_id,
// 								instance->area_id);

// 				struct mospf_lsu * lsu = (struct mospf_lsu *)((char*)mospf_header + MOSPF_HDR_SIZE);
// 				mospf_init_lsu(lsu, nbr_num);
// 				memcpy((char*)((char*)lsu + MOSPF_LSU_SIZE), array, nbr_num * MOSPF_LSA_SIZE);

// 				mospf_header->checksum = mospf_checksum(mospf_header);
// 				ip_send_packet(packet, len);
// 			}
// 		}	
// 		instance->sequence_num++;
// 		free(array);
// 		pthread_mutex_unlock(&mospf_lock);			
// 	}

// 	return NULL;
// }

void handle_mospf_lsu(iface_info_t *iface, char *packet, int len)
{
	fprintf(stdout, "TODO: handle mOSPF LSU message.\n");
	mospf_db_entry_t * entry_pos = NULL;
	struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
	struct mospf_hdr * mospf_head = (struct mospf_hdr *)((char*)ip_hdr + IP_HDR_SIZE(ip_hdr));
	struct mospf_lsu * lsu = (struct mospf_lsu *)((char*)mospf_head + MOSPF_HDR_SIZE);
	struct mospf_lsa * lsa = (struct mospf_lsa *)((char*)lsu + MOSPF_LSU_SIZE);
	u32 rid = ntohl(mospf_head->rid);

	if(instance->router_id == rid){
		return;
	} 
	u32 ip = ntohl(ip_hdr->saddr);
	u16 seq = ntohs(lsu->seq);
	u8 ttl = lsu->ttl;
	u32 nadv = ntohl(lsu->nadv);
	int isFound = 0; 
	pthread_mutex_lock(&mospf_lock);

	//find db entry
	list_for_each_entry(entry_pos, &mospf_db, list) {
		if (entry_pos->rid == rid) {
			// if entry_pos->seq < seq, update db entry
			if (entry_pos->seq < seq) {
				entry_pos->rid = rid;
				entry_pos->seq = seq;
				entry_pos->nadv = nadv;
				entry_pos->alive = 0;
				free(entry_pos->array);
				entry_pos->array = (struct mospf_lsa *)malloc(MOSPF_LSA_SIZE * nadv);
				for (int i = 0; i < nadv; i++) {
					struct mospf_lsa * lsa_pos = (struct mospf_lsa *)((char*)lsa + i * sizeof(struct mospf_lsa));
					entry_pos->array[i].mask = ntohl(lsa_pos->mask);
					entry_pos->array[i].network = ntohl(lsa_pos->network);
					entry_pos->array[i].rid = ntohl(lsa_pos->rid);
				}
			}
			isFound = 1;
			break;
		}
	}

	//if not found, create a new db_entry
	if (!isFound) {
		entry_pos = (mospf_db_entry_t *)malloc(sizeof(mospf_db_entry_t));
		entry_pos->rid = rid;
		entry_pos->seq = seq;
		entry_pos->nadv = nadv;
		entry_pos->alive = 0;
		entry_pos->array = (struct mospf_lsa *)malloc(sizeof(struct mospf_lsa) * nadv);
		for (int i = 0; i < nadv; i++) {
			struct mospf_lsa * lsa_pos = (struct mospf_lsa *)(lsa + i * MOSPF_LSA_SIZE);
			entry_pos->array[i].mask = ntohl(lsa_pos->mask);
			entry_pos->array[i].network = ntohl(lsa_pos->network);
			entry_pos->array[i].rid = ntohl(lsa_pos->rid);
		}
		list_add_tail(&(entry_pos->list), & mospf_db);
	}

	if (isFound) {
		printf("RID\tNetwork\tMask\tNeighbor\n");
		list_for_each_entry(entry_pos, &mospf_db, list){
			for(int i = 0;i < entry_pos->nadv; i++) {
				fprintf(stdout, IP_FMT"\t"IP_FMT"\t"IP_FMT"\t"IP_FMT"\n",
						HOST_IP_FMT_STR(entry_pos->rid),
						HOST_IP_FMT_STR(entry_pos->array[i].network), 
						HOST_IP_FMT_STR(entry_pos->array[i].mask),
						HOST_IP_FMT_STR(entry_pos->array[i].rid)
						);
			}
		}
	}

	//send LSU if ttl > 0
	lsu->ttl--;
	if (lsu->ttl > 0) {
		mospf_nbr_t * nbr_pos = NULL;
		iface_info_t * iface_pos = NULL;
		list_for_each_entry (iface_pos, &instance->iface_list, list) {
			if (iface_pos->index == iface->index) {
				continue;
			}
			nbr_pos = NULL;
			list_for_each_entry (nbr_pos, &iface_pos->nbr_list, list) {
				char * forwarding_packet = (char*) malloc(len);
				memcpy(forwarding_packet, packet, len);
				struct ether_header * eh = (struct ether_header *) forwarding_packet;
				memcpy(eh->ether_shost, iface_pos->mac, ETH_ALEN);

				struct iphdr * iph = packet_to_ip_hdr(forwarding_packet);
				u32 dst_ip = nbr_pos->nbr_ip;
				ip_init_hdr(iph,
							iface_pos->ip,
							dst_ip,
							len - ETHER_HDR_SIZE,
							IPPROTO_MOSPF);

				struct mospf_hdr * mospfh = (struct mospf_hdr *)((char *)iph + IP_HDR_SIZE(iph));
				mospfh->checksum = mospf_checksum(mospfh);
				iph->checksum = ip_checksum(iph);
				ip_send_packet(forwarding_packet, len);
			}
		}
	}
	pthread_mutex_unlock(&mospf_lock);
}

void handle_mospf_packet(iface_info_t *iface, char *packet, int len)
{
	struct iphdr *ip = (struct iphdr *)(packet + ETHER_HDR_SIZE);
	struct mospf_hdr *mospf = (struct mospf_hdr *)((char *)ip + IP_HDR_SIZE(ip));

	if (mospf->version != MOSPF_VERSION) {
		log(ERROR, "received mospf packet with incorrect version (%d)", mospf->version);
		return ;
	}
	if (mospf->checksum != mospf_checksum(mospf)) {
		log(ERROR, "received mospf packet with incorrect checksum, type (%d)", mospf->type);
		return ;
	}
	if (ntohl(mospf->aid) != instance->area_id) {
		log(ERROR, "received mospf packet with incorrect area id");
		return ;
	}

	switch (mospf->type) {
		case MOSPF_TYPE_HELLO:
			handle_mospf_hello(iface, packet, len);
			break;
		case MOSPF_TYPE_LSU:
			handle_mospf_lsu(iface, packet, len);
			break;
		default:
			log(ERROR, "received mospf packet with unknown type (%d).", mospf->type);
			break;
	}
}


void init_graph() {
	memset(router_list, 0, sizeof(router_list));
	router_list[0] = instance->router_id;
	memset(graph, 0, sizeof(graph));
	iface_info_t *iface = NULL;
	int index = 1;
	list_for_each_entry (iface, &instance->iface_list, list) {
		mospf_nbr_t *nbr_pos = NULL;
		list_for_each_entry (nbr_pos, &iface->nbr_list, list) {
			router_list[index] = nbr_pos->nbr_id;
			graph[0][index] = graph[index][0] = 1;
			index++;
		}
	}
}

int min_dist(u16 *dist, int *visited, int num) {
	int index = 0;
	int min = INT16_MAX;
	for (int u = 0; u < num; u++) {
		if (visited[u]) {
			for (int v = 0; v < num; v++) {
				if (visited[v] == 0 && graph[u][v] > 0 && graph[u][v] + dist[u] < min) {
					min = graph[u][v] + dist[u];
					index = v;
				}
			}
		}
	}
	dist[index] = min;
	return index;
}

void Dijkstra(int prev[]) {
	u16 dist[ROUTER_NUM];
	int visited[ROUTER_NUM];
	for(int i = 0; i < ROUTER_NUM; i++) {
		dist[i] = INT16_MAX;
		visited[i] = 0;
	}

	dist[0] = 0;
	visited[0] = 1;

	for(int i = 0; i < ROUTER_NUM; i++) {
		int u = min_dist(dist, visited, ROUTER_NUM);
		visited[u] = 1;
		for (int v = 0; v < ROUTER_NUM; v++){
			if (visited[v] == 0 && graph[u][v] > 0 && dist[u] + graph[u][v] < dist[v]) {
				dist[v] = dist[u] + graph[u][v];
				prev[v] = u;
			}
		}
	}
}

int get_router_list_index(int rid) {
	for (int i = 0; i < ROUTER_NUM; i++) {
		if (rid == router_list[i]) {
			return i;
		}
	}
	return -1;
}

void *generating_rtable_thread(void *param) {
	while (1) {
		fprintf(stdout, "TODO: generate rtable.\n");
		sleep(10);
		pthread_mutex_lock(&mospf_lock);
		init_graph();
		pthread_mutex_unlock(&mospf_lock);
		print_rtable();
		mospf_db_entry_t * db_pos = NULL;

		//update router_list
		pthread_mutex_lock(&mospf_database_lock);
		list_for_each_entry (db_pos, &mospf_db, list) {
			for (int i = 0; i < ROUTER_NUM; i++) {
				if (router_list[i] == 0 || router_list[i] == db_pos->rid) {
					if (router_list[i] == 0) {
						router_list[i] = db_pos->rid;
						fprintf(stdout,"router_list[g1]: " IP_FMT"\t""\n",
							HOST_IP_FMT_STR(router_list[i])
						);
					}
					break;
				}
			}
		}

		//update matrix
		list_for_each_entry (db_pos, &mospf_db, list) {
			int index_1 = get_router_list_index(db_pos->rid);
			if (index_1 == -1) {
				continue;
			}
			for (int i = 0; i < db_pos->nadv; i++) {
				if (db_pos->array[i].rid == 0) {
					continue;
				}
				int index_2 = get_router_list_index(db_pos->array[i].rid);
				if (index_2 == -1) {
					continue;
				}
				graph[index_1][index_2] = graph[index_2][index_1] = 1;
			}
		}
		pthread_mutex_unlock(&mospf_database_lock);
		int prev[ROUTER_NUM];
		memset(prev, 0, sizeof(prev));
		prev[0] = -1;
		Dijkstra(prev);

		// prev -> next hop
		for (int i = 0; i < ROUTER_NUM; i++) {
			if (prev[i] != 0 && prev[prev[i]] != 0) {
				prev[i] = prev[prev[i]];
				i--;
			}
		}

		rt_entry_t *rt_entry = NULL;
		list_for_each_entry (rt_entry, &rtable, list) {
			if (rt_entry->gw ==0 ) {
				rt_entry->check = 1;
			} else {
				rt_entry->check = 0;
			}
		}
		for (int i = 0; i < ROUTER_NUM; i++) {
			u32 rid = router_list[i];
			u32 gw = 0;
			if (prev[i] == 0) {
				gw = router_list[i];
			} else {
				gw = router_list[prev[i]];
			}
			printf("i:%d, prev[i]:%d ",i, prev[i]);
			fprintf(stdout,"rid: " IP_FMT "gw: " IP_FMT"\t""\n",
			HOST_IP_FMT_STR(rid),
							HOST_IP_FMT_STR(gw)
							);
			iface_info_t *iface = NULL;
			mospf_nbr_t *nbr_pos = NULL;
			list_for_each_entry(iface, &instance->iface_list, list) {
				int isFound = 0;
				list_for_each_entry (nbr_pos, &iface->nbr_list, list) {
					if (gw == nbr_pos->nbr_id) {
						isFound = 1;
						fprintf(stdout,"found entry:" IP_FMT"\t""\n",
							HOST_IP_FMT_STR(nbr_pos->nbr_ip)
							);
						break;
					}
				}
				if (isFound) {
					break;
				} 
			}

			mospf_db_entry_t *db_entry = NULL;
			list_for_each_entry (db_entry, &mospf_db, list) {
				if (rid == db_entry->rid) {
					for (int i = 0; i < db_entry->nadv; i++) {
						rt_entry_t *rt_entry = NULL;
						int isFound = 0;
						list_for_each_entry(rt_entry, &rtable, list) {
							if (rt_entry->dest == db_entry->array[i].network) {
								isFound = 1;
								break;
							}
						}
						if (!isFound) {
							if ((db_entry->array[i].mask & (1<<31)) == 0) {
								continue;
							}
							rt_entry_t *new_entry = new_rt_entry(db_entry->array[i].network,
																 db_entry->array[i].mask,
																 nbr_pos->nbr_ip,
																 iface);
							fprintf(stdout,"1new entry:" IP_FMT"\t"IP_FMT"\t"IP_FMT"\t""\n",
							HOST_IP_FMT_STR(db_entry->array[i].network),
							HOST_IP_FMT_STR(db_entry->array[i].mask), 
							HOST_IP_FMT_STR(nbr_pos->nbr_ip)
							);
							new_entry->check = 1;
							add_rt_entry(new_entry);
						} else if (nbr_pos->nbr_ip != rt_entry->gw && rt_entry->check == 0) {
							remove_rt_entry(rt_entry);
							if ((db_entry->array[i].mask & (1<<31)) == 0) {
								continue;
							}
							rt_entry_t *new_entry = new_rt_entry(db_entry->array[i].network,
																 db_entry->array[i].mask,
																 nbr_pos->nbr_ip,
																 iface);
							fprintf(stdout,"2new entry:" IP_FMT"\t"IP_FMT"\t"IP_FMT"\t""\n",
							HOST_IP_FMT_STR(db_entry->array[i].network),
							HOST_IP_FMT_STR(db_entry->array[i].mask), 
							HOST_IP_FMT_STR(nbr_pos->nbr_ip)
							);
							new_entry->check = 1;
							add_rt_entry(new_entry);
						} else {
							rt_entry->check = 1;
						}
						
					}
				}
			}
		}
		print_rtable();
	}
	return NULL;
}