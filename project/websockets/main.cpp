#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080

// Function to read the entire content of a file into a string
std::string readFile(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Determine the Content-Type based on file extension
std::string getContentType(const std::string &path) {
    if (path.find(".html") != std::string::npos)
        return "text/html";
    if (path.find(".css") != std::string::npos)
        return "text/css";
    // Extend for other file types if needed
    return "text/plain";
}

int main() {
    // Create a TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    // Allow reusing the address/port immediately after the server stops
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed.\n";
        return 1;
    }

    // Define the server address
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
    address.sin_port = htons(PORT);

    // Bind the socket to the specified IP and port
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Bind failed.\n";
        return 1;
    }
    
    // Listen for incoming connections
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed.\n";
        return 1;
    }
    
    std::cout << "Server is listening on port " << PORT << "...\n";

    while (true) {
        // Accept a new connection
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Accept failed.\n";
            continue;
        }

        // Read the request into a buffer
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytesReceived < 0) {
            std::cerr << "Read failed.\n";
            close(client_socket);
            continue;
        }

        std::string request(buffer);
        std::cout << "Received request:\n" << request << std::endl;

        // Parse the first line to get the HTTP method and requested path
        std::istringstream requestStream(request);
        std::string method, path, httpVersion;
        requestStream >> method >> path >> httpVersion;
        
        // Default to index.html if the path is "/"
        if (path == "/") {
            path = "/index.html";   
        }
        
        // Build the file path by prepending the current directory
        std::string filePath = "." + path;
        std::string fileContent = readFile(filePath);
        
        if (fileContent.empty()) {
            // File not found; send a 404 response
            std::string notFoundResponse =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<html><body><h1>404 Not Found</h1><p>The requested file was not found.</p></body></html>";
            send(client_socket, notFoundResponse.c_str(), notFoundResponse.size(), 0);
        } else {
            // File found; determine its content type
            std::string contentType = getContentType(filePath);
            // Construct the HTTP response header and body
            std::ostringstream responseStream;
            responseStream << "HTTP/1.1 200 OK\r\n"
                           << "Content-Type: " << contentType << "\r\n"
                           << "Content-Length: " << fileContent.size() << "\r\n"
                           << "Connection: close\r\n"
                           << "\r\n"
                           << fileContent;
            std::string response = responseStream.str();
            send(client_socket, response.c_str(), response.size(), 0);
        }
        
        // Close the client socket after handling the request
        close(client_socket);
    }
    
    // Close the server socket (this line is never reached in this infinite loop)
    close(server_fd);
    return 0;
}
