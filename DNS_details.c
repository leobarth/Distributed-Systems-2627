#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// UR server IP
// #define SERVER_IP "194.94.152.1"
// Google server IP
#define SERVER_IP "8.8.8.8"
// Cloudflare server IP
// #define SERVER_IP "1.1.1.1"
// same port my server uses
#define PORT 53

void parse_dns_response(unsigned char *buf, ssize_t n) {
	// retrieve header values from response

	// typecast buf to short to make indexing easier
	unsigned short *header = (unsigned short *)buf;
	unsigned short resp_flags = ntohs(header[1]); // if 0000 then no error
	unsigned short ancount = ntohs(header[3]); // should be 0001 -> one answer

	printf("ancount = %hu\n", ancount);

	// bitmask to filter for bottom four bits of flags to get RCODE
	unsigned char rcode = resp_flags & 0x0F;
	if (rcode != 0) {
		printf("DNS error: RCODE = %d\n", rcode);
		return;
	}

	// read out question

	int offset = 12; // start from behind the 12-byte header
	// skip over QNAME (domain name)
	while (offset < n && buf[offset] != 0) {
		offset += buf[offset] + 1;
	}
	offset++; // skip over null-byte after the domain name
	offset += 4; // skip QTYPE and QCLASS

	// read out answer, iterating over amount of answers

	for (int i = 0; i < ancount; i++) {
		if ((buf[offset] & 0xC0) == 0xC0) {
			// skip over domain name pointer if present
			offset += 2;
		} else {
			// read out length byte and skip content
			while (offset < n && buf[offset] != 0) {
				offset += buf[offset] + 1;
			}
			offset++; // skip null byte (end indicator)
		}

		// now read out the really interesting fields

		// buf + offset is our current position, and we make the same typecast
		// as with the header way up in this function
		unsigned short type = ntohs(*(unsigned short *)(buf + offset));
		offset += 2;
		// skipped class identifier (not needed here)
		offset += 2;
		unsigned int ttl = ntohl(*(unsigned int *)(buf + offset));
		offset += 4;
		unsigned short rdlength = ntohs(*(unsigned short *)(buf + offset));
		offset += 2;

		// type one means ip address, which is in four-byte format
		if (type == 1 && rdlength == 4) {
			char ip_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, buf + offset, ip_str, sizeof(ip_str));
			printf("Found IP-address: %s (TTL in seconds: %u)\n", ip_str, ttl);
		}
		offset += rdlength;
	}
}

int main() {
	// setting up the socket connection (this I reused and modified from the
	// UDP_Client project)
	int socket_fd;
	struct sockaddr_in server_addr;
	// buffer/stream (octet stream)
	unsigned char buf[512];
	int offset = 0;

	// create UDP socket, as instructed
	socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_fd < 0) {
		perror("Error when creating socket (socket failed)");
		exit(EXIT_FAILURE);
	}

	// prepare server address information
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	// convert string IP text to binary form
	if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
		perror("Error when transforming IP address (inet_pton failed)");
		close(socket_fd);
		exit(EXIT_FAILURE);
	}

	// add binary package information (filling header with generic information)
	unsigned short id = htons(1337);
	memcpy(buf + offset, &id, 2); offset += 2;
	unsigned short flags = htons(0x0100);
	memcpy(buf + offset, &flags, 2); offset += 2;
	unsigned short qdcount = htons(1);
    memcpy(buf + offset, &qdcount, 2); offset += 2;
	unsigned short ancount = htons(0);
    memcpy(buf + offset, &ancount, 2); offset += 2;
    unsigned short nscount = htons(0);
    memcpy(buf + offset, &nscount, 2); offset += 2;
    unsigned short arcount = htons(0);
    memcpy(buf + offset, &arcount, 2); offset += 2;

	// translate hostname string into octet stream by dividing into small
	// pieces between dots
	const char *hostname = "www.google.com";
	const char *tmp = hostname;
	while (*tmp) {
		const char *dot = strchr(tmp, '.');
		int len = dot ? (dot - tmp) : strlen(tmp);
		buf[offset++] = len;
		memcpy(buf + offset, tmp, len);
		offset += len;
		tmp += len;
		if (*tmp == '.') tmp++;
	}
	buf[offset++] = 0;

	// add qtype (1: A type record) and qclass (1: internet address) to package
	unsigned short qtype = htons(1);
	memcpy(buf + offset, &qtype, 2); offset += 2;
	unsigned short qclass = htons(1);
	memcpy(buf + offset, &qclass, 2); offset += 2;

	// send package via UDP
	if (sendto(socket_fd, buf, offset, 0, (struct sockaddr *)&server_addr, 
		sizeof(server_addr)) < 0) {
			perror("Error sending message (sendto failed)");
			close(socket_fd);
			exit(EXIT_FAILURE);
		}

	socklen_t addrlen = sizeof(server_addr);
	ssize_t n = recvfrom(socket_fd, buf, sizeof(buf), 0,
		(struct sockaddr *)&server_addr, &addrlen);
	if (n < 0) {
		perror("Error when receiving message (recvfrom failed)");
		close(socket_fd);
		exit(EXIT_FAILURE);
	}

	parse_dns_response(buf, n);

	close(socket_fd);
	return 0;
}
