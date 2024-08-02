/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        if.h
 * Purpose:     
 * History:     
 *	Jan 17, 2012	jetmotor	Create
 */

#ifndef _IF_H
#define _IF_H


#include "types.h"


#define ETH_ALEN		6	/* Octets in one ethernet addr  */
#define ETH_HLEN		14	/* Total octets in header.      */
#define ETH_ZLEN		60	/* Min. octets in frame sans FCS */
#define ETH_DATA_LEN		1500	/* Max. octets in payload	*/
#define ETH_FRAME_LEN		1514	/* Max. octets in frame sans FCS */
#define ETH_MAX_LEN		1518
#define ETH_VLAN_HLEN		18         
#define ETH_VLAN_MAX_LEN	1496	/* Max data length in VLAN frame */
#define ETH_VLAN_MIN_LEN	42
#define ETH_VLAN_TYPE		0x8100
#define ETH_IPV4_TYPE		0x0800
#define ETH_IPV6_TYPE		0x86dd
#define ETH_BROADCAST_ADDR      "\377\377\377\377\377\377"	/* FF:FF:FF:FF:FF:FF */
#define ETH_ZERO_ADDR           "\000\000\000\000\000\000"	/* 00:00:00:00:00:00 */
#define WILDCARD_ADDR           "\252\252\252\252\252\252"	/* AA:AA:AA:AA:AA:AA */
#define BCAST_NODE_ADDR         "\231\231\231\231\231\231"	/* 99:99:99:99:99:99 */

typedef struct eth_addr_s {
	uint8_t octet[ETH_ALEN];
} __PACKED eth_addr_t;

typedef struct eth_hdr_s {
	uint8_t  dst[ETH_ALEN];		/* destination eth addr */
	uint8_t  src[ETH_ALEN];		/* source ether addr    */
	uint16_t type;			/* packet type ID field */
} __PACKED eth_hdr_t;

typedef struct vlan_tag_s {
	uint16_t vlan_id	:12;
	uint16_t cfi		:1;
	uint16_t pri		:3;

	uint16_t type;
} __PACKED vlan_tag_t;

typedef struct ip_hdr_s {
	char     ver_and_len;
	char     tos;
	uint16_t total_len;
	uint16_t id;
	uint16_t offset;
	char     ttl;
	char     prot;
	uint16_t checksum;
	uint32_t ip_src;
	uint32_t ip_dst;
} __PACKED ip_hdr_t;

typedef struct ipv6_hdr_s {
	uint32_t ver		:4;
	uint32_t ds		:6;
	uint32_t ecn		:2;
	uint32_t flow_label	:20;
	uint16_t payload_len;
	uint16_t next_hdr	:8;
	uint16_t hop_limit	:8;
	uint32_t ip_src[4];
	uint32_t ip_dst[4];
} __PACKED ipv6_hdr_t;

typedef struct arp_hdr_s {
	uint16_t hard_type;
	uint16_t prot_type;
	char     hard_size;
	char     prot_size;
	uint16_t op;
	char     sender_mac[6];
	uint32_t sender_ip;
	char     target_mac[6];
	uint32_t target_ip;
} __PACKED arp_hdr_t;

typedef struct udp_hdr_s {
	uint16_t src_port;
	uint16_t dest_port;
	uint16_t len;
	uint16_t checksum;
} __PACKED udp_hdr_t;

typedef struct arp_table_s {
	uint32_t ip;
	char     mac[6];
} arp_table_t;


#endif	/* end of _IF_H */
