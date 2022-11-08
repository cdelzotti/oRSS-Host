/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <time.h>

#include "../env.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */

/* Main functional part of port initialization. 8< */
static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	memset(&port_conf, 0, sizeof(struct rte_eth_conf));

	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval != 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-retval));
		return retval;
	}

	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
				rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	/* Starting Ethernet port. 8< */
	retval = rte_eth_dev_start(port);
	/* >8 End of starting of ethernet port. */
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval != 0)
		return retval;

	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port, RTE_ETHER_ADDR_BYTES(&addr));

	/* Enable RX in promiscuous mode for the Ethernet device. */
	retval = rte_eth_promiscuous_enable(port);
	/* End of setting RX port in promiscuous mode. */
	if (retval != 0)
		return retval;

	return 0;
}
/* >8 End of main functional part of port initialization. */

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */

 /* Basic forwarding application lcore. 8< */
static __rte_noreturn void
lcore_main(void)
{
	uint16_t port;

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	/* Main work of application loop. 8< */
	for (;;) {
		/*
		 * Receive packets on a port and forward them on the paired
		 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
		 */

		// For each port
		RTE_ETH_FOREACH_DEV(port) {
			// Check if port is up
			if (!rte_eth_dev_is_valid_port(port)) {
				printf("Port %d is not valid\n", port);
				continue;
			}

			/* Get burst of RX packets, from first port of pair. */
			struct rte_mbuf *bufs[BURST_SIZE];
			const uint16_t nb_rx = rte_eth_rx_burst(port, 0,
					bufs, BURST_SIZE);

			if (unlikely(nb_rx == 0)){
				continue;
			}

			/* Send burst of TX packets, to second port of pair. */
			//const uint16_t nb_tx = rte_eth_tx_burst(port ^ 1, 0,
			//		bufs, nb_rx);
			int nb_tx = 0;
			if (unlikely(nb_tx < nb_rx)) {
				uint16_t buf;
				for (buf = nb_tx; buf < nb_rx; buf++){
					// Parse ethernet header with vlan
					struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(bufs[buf], struct rte_ether_hdr *);
					struct rte_vlan_hdr *vlan_hdr = (struct rte_vlan_hdr *)(eth_hdr + 1);
					printf("Oh, I think I just saw a packet with VLAN %u and proto %u\n", rte_be_to_cpu_16(vlan_hdr->vlan_tci), rte_be_to_cpu_16(vlan_hdr->eth_proto));

					if (unlikely(vlan_hdr->eth_proto != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4))){
						rte_pktmbuf_free(bufs[buf]);
						continue;
					}
					
					// printf("Received MAC src %u:%u:%u:%u:%u:%u, dst %u:%u:%u:%u:%u:%u\n", 
					//eth_hdr->src_addr.addr_bytes[0], 
					//eth_hdr->src_addr.addr_bytes[1], 
					//eth_hdr->src_addr.addr_bytes[2], 
					//eth_hdr->src_addr.addr_bytes[3], 
					//eth_hdr->src_addr.addr_bytes[4], 
					//eth_hdr->src_addr.addr_bytes[5], 
					//eth_hdr->dst_addr.addr_bytes[0], 
					//eth_hdr->dst_addr.addr_bytes[1], 
					//eth_hdr->dst_addr.addr_bytes[2], 
					//eth_hdr->dst_addr.addr_bytes[3], 
					//eth_hdr->dst_addr.addr_bytes[4], 
					//eth_hdr->dst_addr.addr_bytes[5]);
					// Parse ipv4 header
					struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)(vlan_hdr + 1);
					//uint32_t src_ip = rte_be_to_cpu_32(ipv4_hdr->src_addr);
					//printf("I saw something on port %u from", port);
					//printf(" %u\n", src_ip);
					// Check if the packet is UDP
					if (ipv4_hdr->next_proto_id == IPPROTO_UDP){
						// Parse UDP header
						struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)(ipv4_hdr + 1);
						// Get source and destination port
						uint16_t src_port = rte_be_to_cpu_16(udp_hdr->src_port);
						uint16_t dst_port = rte_be_to_cpu_16(udp_hdr->dst_port);
						// Print flow information
						// printf("UDP packet from port %u to port %u with VLAN %u\n", src_port, dst_port, rte_be_to_cpu_16(vlan_hdr->vlan_tci));
						// Parse payload
						//	struct payload *payload = (struct payload *)(udp_hdr + 1);
						// Print payload
						//	printf("Payload: %u\n", payload->data);
					} else {
					//	printf("Not UDP\n");
					}
					rte_pktmbuf_free(bufs[buf]);
				}
			}
		}
	}
	/* >8 End of loop. */
}
/* >8 End Basic forwarding application lcore. */

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	unsigned nb_ports;
	uint16_t portid;

	/* Initializion the Environment Abstraction Layer (EAL). 8< */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	/* >8 End of initialization the Environment Abstraction Layer (EAL). */

	argc -= ret;
	argv += ret;

	/* Check that there is an even number of ports to send/receive on. */
	nb_ports=1;
	//nb_ports = rte_eth_dev_count_avail();
	//if (nb_ports < 2 || (nb_ports & 1))
	//	rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");

	/* Creates a new mempool in memory to hold the mbufs. */

	/* Allocates mempool to hold the mbufs. 8< */
	printf("Init MEMPOOL\n");
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	/* >8 End of allocating mempool to hold mbuf. */

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initializing all ports. 8< */
	RTE_ETH_FOREACH_DEV(portid){
		printf("Initializing port %u\n", portid);
		if (port_init(portid, mbuf_pool) != 0){
			printf("Failed to init port %d", portid);
		 	rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n",
					portid);
		}
	}
	/* >8 End of initializing all ports. */

	// if (rte_lcore_count() > 1)
	// 	printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	// /* Call lcore_main on the main core only. Called on single lcore. 8< */
	lcore_main();
	/* >8 End of called on single lcore. */

	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}
