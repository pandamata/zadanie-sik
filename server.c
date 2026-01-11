#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <errno.h>

#define PORT 8443
#define BUFFER_SIZE 1024

// Function to initialize OpenSSL (modern way - OpenSSL 1.1.0+)
void init_openssl() {
    // In OpenSSL 1.1.0+, initialization is done automatically
    // SSL_load_error_strings() is kept for better error reporting
    SSL_load_error_strings();
}

// Function to create and configure the SSL context
SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Enforce minimum TLS 1.2
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

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

// Function to send all data through SSL, handling partial writes
int send_all(SSL *ssl, const char *data, size_t len) {
    size_t total_sent = 0;
    
    while (total_sent < len) {
        int sent = SSL_write(ssl, data + total_sent, len - total_sent);
        if (sent <= 0) {
            int err = SSL_get_error(ssl, sent);
            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                // Retry - socket is temporarily unavailable
                continue;
            } else {
                // Real error occurred
                ERR_print_errors_fp(stderr);
                return -1;
            }
        }
        total_sent += sent;
    }
    
    return 0;
}

// Function to handle each client connection and serve the HTML file over SSL
void handle_client(SSL *ssl, int client_socket __attribute__((unused))) {
    struct stat file_stat;
    FILE *html_file;
    char *file_content = NULL;
    char header[512];
    int header_len;

    // Get file size using stat()
    if (stat("index.html", &file_stat) != 0) {
        perror("Could not stat HTML file");
        return;
    }
    
    size_t file_size = file_stat.st_size;

    // Open the HTML file to be served
    html_file = fopen("index.html", "rb");
    if (html_file == NULL) {
        perror("Could not open HTML file");
        return;
    }

    // Allocate memory for entire file
    file_content = (char *)malloc(file_size);
    if (file_content == NULL) {
        perror("Could not allocate memory for file");
        fclose(html_file);
        return;
    }

    // Read entire file at once
    size_t bytes_read = fread(file_content, 1, file_size, html_file);
    fclose(html_file);
    
    if (bytes_read != file_size) {
        perror("Could not read entire file");
        free(file_content);
        return;
    }

    // Send HTTP headers with proper Content-Type, Content-Length, and Connection
    header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        file_size);
    
    if (send_all(ssl, header, header_len) != 0) {
        fprintf(stderr, "Failed to send headers\n");
        free(file_content);
        return;
    }

    // Send the entire file content
    if (send_all(ssl, file_content, file_size) != 0) {
        fprintf(stderr, "Failed to send file content\n");
        free(file_content);
        return;
    }

    free(file_content);
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

    // Set socket options - only SO_REUSEADDR
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
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
            // Clean up SSL and socket on handshake failure
            SSL_free(ssl);
            close(client_socket);
        } else {
            // Handle the client connection and serve the HTML file over SSL
            handle_client(ssl, client_socket);
            
            // Proper connection shutdown
            // 1. Call SSL_shutdown
            SSL_shutdown(ssl);
            
            // 2. Drain any remaining data using recv() in a loop
            char drain_buffer[256];
            ssize_t n;
            while ((n = recv(client_socket, drain_buffer, sizeof(drain_buffer), MSG_DONTWAIT)) > 0) {
                // Just drain the data
            }
            // Note: recv returns -1 with errno==EAGAIN/EWOULDBLOCK when done,
            // or 0 on clean shutdown, both are OK here
            
            // 3. Call shutdown on the socket
            shutdown(client_socket, SHUT_RDWR);
            
            // 4. Free SSL and close socket
            SSL_free(ssl);
            close(client_socket);
        }
    }

    // Close the server socket and clean up
    close(server_fd);
    SSL_CTX_free(ctx);
    // EVP_cleanup() is no longer needed in OpenSSL 1.1.0+
    return 0;
}
