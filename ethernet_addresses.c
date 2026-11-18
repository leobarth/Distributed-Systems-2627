#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <string.h>

// documentary-based code, reference the linux manual at:
// https://man7.org/linux/man-pages/man3/getifaddrs.3.html
int main() {
	// ifaddrs struct: access to linked list of interface addresses
	struct ifaddrs *ifaddr, *ifa;

	// error handling: getifaddrs
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddr failed");
		exit(EXIT_FAILURE);
	}

	// walk through the linked list
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}

		// linux way of accessing the physical-layer socket addresses;
		// once again, reference the linux manual at:
		// https://man7.org/linux/man-pages/man7/packet.7.html
		if (ifa->ifa_addr->sa_family == AF_PACKET) {
			// type casting to access full address;
			// ll stands for linked layer, i.e. the 2nd layer in the OSI-system
			struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;

			// check for correct length to filter for MAC-addresses
			if (s->sll_halen == 6) {
				printf("Interface: %s \t Ethernet-/MAC-Address: ", ifa->ifa_name);
				printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
						s->sll_addr[0], s->sll_addr[1], s->sll_addr[2],
						s->sll_addr[3], s->sll_addr[4], s->sll_addr[5]);
			}
		}
	}

	freeifaddrs(ifaddr);
	return 0;
}
