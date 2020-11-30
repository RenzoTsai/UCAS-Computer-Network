#include "mospf_daemon.h"
#include "mospf_proto.h"
#include "mospf_nbr.h"
#include "mospf_database.h"

#include "ip.h"

#include "list.h"
#include "log.h"
#include "packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

extern ustack_t *instance;

pthread_mutex_t mospf_lock;

void mospf_init()
{
	pthread_mutex_init(&mospf_lock, NULL);

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
void *checking_nbr_thread(void *param);
void *checking_database_thread(void *param);

void mospf_run()
{
	pthread_t hello, lsu, nbr, db;
	pthread_create(&hello, NULL, sending_mospf_hello_thread, NULL);
	pthread_create(&lsu, NULL, sending_mospf_lsu_thread, NULL);
	pthread_create(&nbr, NULL, checking_nbr_thread, NULL);
	pthread_create(&db, NULL, checking_database_thread, NULL);
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
					list_delete_entry(&nbr_pos->list);
					free(nbr_pos);
					fprintf(stdout, "TODO: free a neighbor node.\n");
					iface->num_nbr--;
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
		mospf_nbr_t * db_pos = NULL, *db_q = NULL;
		iface_info_t *iface = NULL;
		pthread_mutex_lock(&mospf_lock);
		list_for_each_entry(db_pos, &mospf_db, list) {
			db_pos->alive++;
			if (db_pos->alive > MOSPF_DATABASE_TIMEOUT) {
				list_delete_entry(&db_pos->list);
				free(db_pos);
				fprintf(stdout, "TODO: free a database node.\n");
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

void *sending_mospf_lsu_thread(void *param)
{
	fprintf(stdout, "TODO: send mOSPF LSU message periodically.\n");
	while (1){
		sleep(MOSPF_DEFAULT_LSUINT);
		pthread_mutex_lock(&mospf_lock);
		iface_info_t * iface = NULL;
		int nbr_num = 0;
		list_for_each_entry (iface, &instance->iface_list, list) { 
			if (iface->num_nbr == 0) {
				nbr_num ++;
			} else {
				nbr_num += iface->num_nbr;
			}	
		}
		pthread_mutex_unlock(&mospf_lock);

		struct mospf_lsa * array = (struct mospf_lsa *)malloc(nbr_num * MOSPF_LSA_SIZE);
		mospf_nbr_t * nbr_pos = NULL;

		int index = 0;
		pthread_mutex_lock(&mospf_lock);
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
		pthread_mutex_unlock(&mospf_lock);			
	}

	return NULL;
}

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
