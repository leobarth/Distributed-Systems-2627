#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error when creating socket");
        exit(EXIT_FAILURE);
    }

    // ensures reusing address on quick restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket to port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error when binding socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // set into listen mode
    if (listen(server_fd, 10) < 0) {
        perror("Error when listening");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("HTTP-Server is running on port %d...\n", PORT);

    // infinite loop for incoming traffic
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Error when accepting");
            continue;
        }

        // read client request
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        // define HTML body
        char *body = 
                    "<!DOCTYPE html><html>"
                    "<head><title>Title of the document</title>"
                    "<style>.big { font-size: larger; }</style>"
                    "</head>"
                    "<body>"
                    "<p>This text is normal, <span class=\"big\">and this text is bigger.</span></p>"
                    "</body>"
                    "</html>\r\n";

        // build HTTP response including dynamic content length
        char http_response[512];
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %lu\r\n"
            "\r\n"
            "%s", strlen(body), body);

        // send response to client
        send(client_socket, http_response, strlen(http_response), 0);

        // instantly close socket to let client know the transmission ended
        close(client_socket);
    }
}
