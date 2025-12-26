/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleClients.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/23 09:42:42 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/26 00:00:00 by refactor         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "handleClients.hpp"
#include "Cgi.hpp"
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <ctime>
#include <csignal>

// External signal flag from serv.cpp
extern volatile sig_atomic_t g_running;

// ============================================================================
// CONSTANTS
// ============================================================================

static const int    MAX_EVENTS = 64;
static const int    EPOLL_TIMEOUT_MS = 1000;
static const int    CLIENT_TIMEOUT_SEC = 60;
static const size_t READ_BUFFER_SIZE = 8192;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Find a server by its socket file descriptor
 */
static HttpServer* findServerByFd(std::vector<HttpServer> &servers, int fd)
{
    for (std::vector<HttpServer>::iterator it = servers.begin();
         it != servers.end(); ++it)
    {
        if (it->getSocketFd() == fd)
            return &(*it);
    }
    return NULL;
}

/**
 * Safely close a client connection and remove from tracking
 */
static void closeClient(int epoll_fd, std::map<int, Client> &clients, int client_fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    clients.erase(client_fd);
}

/**
 * Set a file descriptor to non-blocking mode
 */
static bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

// ============================================================================
// CLIENT MANAGEMENT
// ============================================================================

/**
 * Accept a new client connection
 * Sets the socket to non-blocking and registers with epoll
 */
static void acceptNewClient(int server_fd, int epoll_fd, 
                            std::map<int, Client> &clients, 
                            HttpServer* server)
{
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "accept() failed: " << strerror(errno) << std::endl;
        return;
    }
    
    // Set client socket to non-blocking (required by subject)
    if (!setNonBlocking(client_fd))
    {
        close(client_fd);
        return;
    }
    
    // Create and track new client
    Client client(client_fd, server_fd);
    client.setServerPtr(server);
    client.time = std::time(NULL);
    clients.insert(std::make_pair(client_fd, client));
    
    // Register with epoll for read events
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    ev.data.fd = client_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
    {
        close(client_fd);
        clients.erase(client_fd);
    }
}

/**
 * Check and disconnect clients that have timed out
 * Prevents resource exhaustion from idle connections
 */
void checkTimeout(std::map<int, Client> &clients, int epoll_fd)
{
    time_t now = std::time(NULL);
    std::vector<int> toRemove;
    
    for (std::map<int, Client>::iterator it = clients.begin();
         it != clients.end(); ++it)
    {
        time_t duration = now - static_cast<time_t>(it->second.time);
        if (duration >= CLIENT_TIMEOUT_SEC)
            toRemove.push_back(it->first);
    }
    
    for (size_t i = 0; i < toRemove.size(); i++)
    {
        closeClient(epoll_fd, clients, toRemove[i]);
    }
}

// ============================================================================
// MULTIPART FORM DATA HANDLING
// ============================================================================

/**
 * Extract filename from Content-Disposition header
 */
static std::string extractFilename(const std::string& header)
{
    std::string toSearch = "filename=\"";
    size_t pos = header.find(toSearch);
    if (pos == std::string::npos)
        return "";
    
    pos += toSearch.size();
    size_t endPos = header.find("\"", pos);
    if (endPos == std::string::npos)
        return "";
    
    return header.substr(pos, endPos - pos);
}

/**
 * Handle multipart/form-data uploads
 */
static void handleMultipart(Client &client, const Location &location)
{
    const std::string boundary = client.request.boundary;
    const std::string headerEnd = "\r\n\r\n";
    std::string &body = client.response.body;
    
    size_t boundPos;
    while ((boundPos = body.find(boundary)) != std::string::npos)
    {
        // Check for end boundary
        if (body.size() > boundPos + boundary.size() &&
            body[boundPos + boundary.size()] == '-')
        {
            client.response.multipartEnd = true;
            break;
        }
        
        // Skip boundary and CRLF
        size_t partStart = boundPos + boundary.size();
        if (partStart + 2 <= body.size() && body.substr(partStart, 2) == "\r\n")
            partStart += 2;
        
        // Find headers end
        size_t headersEnd = body.find(headerEnd, partStart);
        if (headersEnd == std::string::npos)
            break;
        
        // Extract filename
        std::string headers = body.substr(partStart, headersEnd - partStart);
        std::string filename = extractFilename(headers);
        if (filename.empty())
        {
            body = body.substr(headersEnd + headerEnd.size());
            continue;
        }
        
        // Find content boundaries
        size_t contentStart = headersEnd + headerEnd.size();
        size_t nextBoundary = body.find(boundary, contentStart);
        if (nextBoundary == std::string::npos)
            break;
        
        // Extract content (remove trailing CRLF before next boundary)
        size_t contentEnd = nextBoundary;
        if (contentEnd >= 2 && body.substr(contentEnd - 2, 2) == "\r\n")
            contentEnd -= 2;
        
        std::string content = body.substr(contentStart, contentEnd - contentStart);
        
        // Determine upload path
        std::string uploadPath = location.getUploadPath();
        if (uploadPath.empty())
            uploadPath = "./uploadFolder/";
        if (uploadPath[uploadPath.size() - 1] != '/')
            uploadPath += "/";
        
        std::string fullPath = uploadPath + filename;
        
        // Write file (binary mode for safety)
        std::ofstream outFile(fullPath.c_str(), std::ios::binary | std::ios::app);
        if (outFile.is_open())
        {
            outFile.write(content.c_str(), content.size());
            outFile.close();
        }
        
        // Move past this part
        body = body.substr(nextBoundary);
    }
}

// ============================================================================
// RESPONSE HANDLING
// ============================================================================

/**
 * Send an HTTP redirect response
 */
static void sendRedirect(Client &client, int code, const std::string &url)
{
    std::stringstream ss;
    ss << "HTTP/1.1 " << code << " Redirect\r\n";
    ss << "Location: " << url << "\r\n";
    ss << "Connection: close\r\n";
    ss << "\r\n";
    
    std::string response = ss.str();
    send(client.client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    client.response.full = true;
}

/**
 * Send a CGI response
 */
static void sendCgiResponse(Client &client)
{
    std::stringstream ss;
    ss << "HTTP/1.1 " << client.response.statusCode << " OK\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Content-Length: " << client.response.body.size() << "\r\n";
    ss << "Connection: close\r\n";
    ss << "\r\n";
    ss << client.response.body;
    
    std::string response = ss.str();
    send(client.client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    client.response.full = true;
}

/**
 * Send a directory listing response
 */
static void sendDirectoryListing(Client &client, Location &location)
{
    std::string content = indexof(location, client.request.path);
    send(client.client_fd, content.c_str(), content.size(), MSG_NOSIGNAL);
    client.response.full = true;
}

/**
 * Process and send response to client
 * Main response routing logic
 */
static void processResponse(std::map<int, Client> &clients, Client &client,
                           int epoll_fd, struct epoll_event &events)
{
    (void)clients;
    (void)epoll_fd;
    (void)events;
    
    if (!client.server_ptr)
    {
        client.response.statusCode = 500;
        client.error();
        return;
    }
    
    Location &location = client.server_ptr->getLocation(client.request.rawPath);
    
    // 1. Handle configured redirects
    if (location.getRedirectCode() != 0)
    {
        sendRedirect(client, location.getRedirectCode(), location.getRedirectUrl());
        return;
    }
    
    // 2. Check if method is allowed
    if (!location.isAllowedMethod(client.request.methodName))
    {
        client.response.statusCode = 405;
        client.error();
        return;
    }
    
    // 3. Handle CGI requests
    if (isCgiRequest(client.request.path, location) && !location.getCgiPath().empty())
    {
        handleCgi(client, *client.server_ptr, epoll_fd, events);
        
        if (client.response.statusCode != 200)
        {
            client.error();
            return;
        }
        
        sendCgiResponse(client);
        return;
    }
    
    // 4. Handle directory requests
    if (mime(client.request.path) == FOLDER && client.request.methodName == "GET")
    {
        if (!location.getAutoIndex())
        {
            // Redirect to index file
            client.redirect(client.request.rawPath + location.getIndex());
            return;
        }
        
        sendDirectoryListing(client, location);
        return;
    }
    
    // 5. Handle DELETE method
    if (client.response.method == "DELETE" && client.response.statusCode == 200)
    {
        client.response.deleteMethod(client.request.path);
    }
    
    // 6. Set MIME type
    client.response.mimeType = client.response.getRightMimeType(client.request.path);
    
    // 7. Handle errors
    if (client.response.statusCode != 200 && client.response.statusCode != 201)
    {
        client.error();
        return;
    }
    
    // 8. Open file for GET
    if (!client.response.file.is_open() && client.response.method != "DELETE")
    {
        client.response.openFile(client.request.path);
    }
    
    // 9. Handle POST (file upload)
    if (client.request.methodName == "POST")
    {
        if (!client.request.multipart)
            client.response.uploadBody();
        
        client.response.method = "GET";
        client.response.mimeType = "html";
        
        std::string responsePage = (client.response.statusCode == 201) 
            ? "./Post/201.html" 
            : "./Post/200.html";
        client.request.path = responsePage;
        
        if (!client.response.file.is_open())
            client.response.openFile(client.request.path);
    }
    
    // 10. Send response
    std::string response;
    if (!client.response.header)
        response = client.response.createResponse(client.request);
    else
        response = client.response.rightMethod();
    
    ssize_t sent = send(client.client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    if (sent <= 0)
    {
        client.response.full = true;
    }
}

// ============================================================================
// REQUEST PARSING
// ============================================================================

/**
 * Parse incoming request headers
 * Returns true if headers are complete
 */
static bool parseRequestHeaders(Client &client, HttpServer *server)
{
    if (client.entry.find("\r\n\r\n") == std::string::npos)
        return false;
    
    client.checkHead();
    
    Location location = server->getLocation(client.request.path);
    client.request.rawPath = client.request.path;
    client.setServerPtr(client.server_ptr);
    client.checkPath();
    client.request.fullHeader = true;
    
    return true;
}

/**
 * Read POST body data
 * Returns: 1 = continue reading, 0 = complete, -1 = error/close
 */
static int readPostBody(Client &client, int epoll_fd, std::map<int, Client> &clients)
{
    char buffer[READ_BUFFER_SIZE];
    ssize_t bytes_read = read(client.client_fd, buffer, sizeof(buffer));
    
    if (bytes_read > 0)
    {
        client.response.body.append(buffer, bytes_read);
        client.request.body.append(buffer, bytes_read);
    }
    else if (bytes_read == 0)
    {
        // Connection closed by client
        closeClient(epoll_fd, clients, client.client_fd);
        return -1;
    }
    else if (errno != EAGAIN && errno != EWOULDBLOCK)
    {
        // Read error
        closeClient(epoll_fd, clients, client.client_fd);
        return -1;
    }
    
    // Check if we have all data
    if (client.response.body.size() >= client.response.contentLength)
    {
        client.request.full = true;
        client.request.body = client.response.body;
        return 0;
    }
    
    return 1; // Continue reading
}

// ============================================================================
// MAIN EVENT LOOP
// ============================================================================

/**
 * Handle EPOLLIN event (data ready to read)
 */
static void handleReadEvent(int fd, int epoll_fd, std::map<int, Client> &clients,
                           std::vector<HttpServer> &servers)
{
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client &client = it->second;
    HttpServer *server = findServerByFd(servers, client.server_fd);
    
    if (!server)
    {
        client.response.statusCode = 500;
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = client.client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client.client_fd, &ev);
        return;
    }
    
    // Read headers if not complete
    if (!client.request.fullHeader)
    {
        char buffer[READ_BUFFER_SIZE];
        ssize_t bytes_read = read(client.client_fd, buffer, sizeof(buffer));
        
        if (bytes_read <= 0)
        {
            if (bytes_read == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
            {
                closeClient(epoll_fd, clients, client.client_fd);
                return;
            }
            return; // EAGAIN, try again later
        }
        
        client.entry.append(buffer, bytes_read);
        
        if (!parseRequestHeaders(client, server))
            return; // Headers not complete yet
    }
    
    // Handle POST body
    if ((client.response.statusCode == 200 || client.response.statusCode == 201) &&
        client.request.methodName == "POST" && !client.request.full)
    {
        int result = readPostBody(client, epoll_fd, clients);
        if (result == -1)
            return; // Client removed
        
        if (client.request.multipart)
        {
            Location &location = server->getLocation(client.request.rawPath);
            handleMultipart(client, location);
            
            if (client.response.multipartEnd)
                client.request.full = true;
        }
        
        if (!client.request.full)
            return; // Need more data
    }
    
    // Update timestamp and switch to write mode
    client.time = std::time(NULL);
    struct epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.fd = client.client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client.client_fd, &ev);
}

/**
 * Handle EPOLLOUT event (ready to write)
 */
static void handleWriteEvent(int fd, int epoll_fd, std::map<int, Client> &clients,
                            struct epoll_event &events)
{
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client &client = it->second;
    processResponse(clients, client, epoll_fd, events);
    
    if (client.response.full)
        closeClient(epoll_fd, clients, client.client_fd);
}

/**
 * Main event loop
 * Single poll() for all I/O operations as required by subject
 */
void multiple(std::vector<HttpServer> &servers)
{
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        std::cerr << "Failed to create epoll instance: " << strerror(errno) << std::endl;
        return;
    }
    
    std::map<int, Client> clients;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    
    // Register all server sockets
    bool setup_failed = false;
    for (std::vector<HttpServer>::iterator it = servers.begin();
         it != servers.end(); ++it)
    {
        if (it->getSocketFd() < 0)
        {
            setup_failed = true;
            break;
        }
        it->setToEppoll(epoll_fd, ev);
    }
    
    if (setup_failed)
    {
        std::cerr << "Server setup failed, cleaning up..." << std::endl;
        close(epoll_fd);
        return;
    }
    
    // Main event loop - runs until SIGINT/SIGTERM
    while (g_running)
    {
        // Periodically check for timed-out clients
        checkTimeout(clients, epoll_fd);
        
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT_MS);
        
        if (nfds == -1)
        {
            if (errno == EINTR)
                continue; // Interrupted by signal, retry
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
            break;
        }
        
        if (nfds == 0)
            continue; // Timeout, loop back
        
        // Process all ready file descriptors
        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;
            uint32_t evts = events[i].events;
            
            // Check if this is a server socket (new connection)
            HttpServer *server = findServerByFd(servers, fd);
            if (server != NULL)
            {
                acceptNewClient(fd, epoll_fd, clients, server);
                continue;
            }
            
            // Handle client events
            if (evts & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                // Error or disconnect
                closeClient(epoll_fd, clients, fd);
                continue;
            }
            
            if (evts & EPOLLIN)
            {
                handleReadEvent(fd, epoll_fd, clients, servers);
            }
            else if (evts & EPOLLOUT)
            {
                handleWriteEvent(fd, epoll_fd, clients, ev);
            }
        }
    }
    
    // ========================================================================
    // CLEANUP - Free all resources on shutdown
    // ========================================================================
    
    std::cout << "Cleaning up resources..." << std::endl;
    
    // Close all client connections
    for (std::map<int, Client>::iterator it = clients.begin();
         it != clients.end(); ++it)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
        close(it->first);
    }
    clients.clear();
    
    // Close all server sockets
    for (std::vector<HttpServer>::iterator it = servers.begin();
         it != servers.end(); ++it)
    {
        int server_fd = it->getSocketFd();
        if (server_fd >= 0)
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, NULL);
            close(server_fd);
        }
    }
    
    // Close epoll instance
    close(epoll_fd);
    
    std::cout << "Server shutdown complete." << std::endl;
}

