#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// work on user ports and use moderate buffer size
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
	int socket_file_descriptor;
	// sockaddr_in is the IPv4-specific format;
	// cast to sockaddr in method calls later
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	char buffer[BUFFER_SIZE];

	// create a UDP socket
	socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_file_descriptor < 0) {
		perror("Error when creating the socket");
		exit(EXIT_FAILURE);
	}

	// initialize server address using memset
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind socket to port
	int bind_success = bind(socket_file_descriptor,
		(struct sockaddr *)&server_addr, sizeof(server_addr));
	if (bind_success < 0) {
		perror("Error when binding socket to port");
		close(socket_file_descriptor);
		exit(EXIT_FAILURE);
	}

	printf("UDP Server is up and running, waiting for port %d...\n", PORT);

	// listen to messages on the port
	while (1) {
		// initialize buffer
		memset(buffer, 0, BUFFER_SIZE);

		// recvfrom is a blocking call;
		// subtracting 1 from BUFFER_SIZE for possible string terminators;
		// ssize_t is specifically constructed for function returns of varying
		// size (unsigned int) and possible errors (-1)
		ssize_t bytes_received = recvfrom(socket_file_descriptor, buffer,
			BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr,
			&client_addr_len);
		if (bytes_received < 0) {
			perror("Error when receiving data (recvfrom)");
			break;
		}

		printf("Received message: %s\n", buffer);
	}

	close(socket_file_descriptor);
	return 0;
}
