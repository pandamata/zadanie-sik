#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8443
#define BUFFER_SIZE 1024

// Function to initialize OpenSSL
void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// Function to create and configure the SSL context
SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Load certificate and private key
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

// Function to handle each client connection and serve the HTML file over SSL
void handle_client(SSL *ssl) {
    FILE *html_file;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Open the HTML file to be served
    html_file = fopen("index.html", "r");
    if (html_file == NULL) {
        perror("Could not open HTML file");
        SSL_shutdown(ssl);
        SSL_free(ssl);
        return;
    }

    // Send HTTP headers to the client
    const char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    SSL_write(ssl, header, strlen(header));

    // Send the contents of the HTML file
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), html_file)) > 0) {
        SSL_write(ssl, buffer, bytes_read);
    }

    // Close the HTML file and SSL connection
    fclose(html_file);
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    SSL_CTX *ctx;

    // Initialize OpenSSL and create SSL context
    init_openssl();
    ctx = create_context();

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setting socket options failed");
        close(server_fd);
        SSL_CTX_free(ctx);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        SSL_CTX_free(ctx);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 10) < 0) {
        perror("Listening failed");
        close(server_fd);
        SSL_CTX_free(ctx);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d (SSL enabled)...\n", PORT);

    // Main loop to accept and handle clients
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        // Create an SSL structure for the connection
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_socket);

        // Perform SSL handshake
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            // Handle the client connection and serve the HTML file over SSL
            handle_client(ssl);
        }

        close(client_socket);
    }

    // Close the server socket and clean up
    close(server_fd);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    return 0;
}
