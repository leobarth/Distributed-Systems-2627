#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// private IP
#define SERVER_IP "127.0.0.1"
// same port my server uses
#define PORT 8080

int main() {
	int socket_file_descriptor;
	struct sockaddr_in server_addr;
	const char *message = "Hello World!";

	// create UDP socket, as instructed
	socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_file_descriptor < 0) {
		perror("Error when creating socket (socket failed)");
		exit(EXIT_FAILURE);
	}

	// prepare server address information
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	// convert string IP text to binary form
	int inet_pton_success = inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
	if (inet_pton_success < 0) {
		perror("Error when transforming IP address (inet_pton failed)");
		close(socket_file_descriptor);
		exit(EXIT_FAILURE);
	}

	// use the socket API's sendto() function;
	// take note of the type cast into a generic sockaddr struct pointer
	// (socket API functions require protocol-agnostic arguments)
	ssize_t bytes_sent = sendto(socket_file_descriptor, message,
		strlen(message), 0, (struct sockaddr *)&server_addr,
		sizeof(server_addr));
	if (bytes_sent < 0) {
		perror("Error sending message (sendto failed)");
	}
	else {
		printf("Successfully sent message (%ld Bytes).", bytes_sent);
	}

	close(socket_file_descriptor);
	return 0;
}
