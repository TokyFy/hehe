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
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <ctime>
#include <csignal>

extern volatile sig_atomic_t g_running;

static const int    MAX_EVENTS = 64;
static const int    EPOLL_TIMEOUT_MS = 1000;
static const int    CLIENT_TIMEOUT_SEC = 60;
static const size_t READ_BUFFER_SIZE = 8192;

static void logRequest(int status, const std::string &method, const std::string &path)
{
    const char* color = "\033[32m";
    if (status >= 400) color = "\033[31m";
    else if (status >= 300) color = "\033[33m";
    
    std::cout << color << status << "\033[0m " << method << " " << path << std::endl;
}

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

static void closeClient(int epoll_fd, std::map<int, Client> &clients, int client_fd)
{
    std::map<int, Client>::iterator it = clients.find(client_fd);
    if (it != clients.end())
    {
        // Clean up CGI if active
        if (it->second.cgi.active)
            cleanupCgi(it->second, epoll_fd);
    }
    
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    clients.erase(client_fd);
}

static void acceptNewClient(int server_fd, int epoll_fd, 
                            std::map<int, Client> &clients, 
                            HttpServer* server)
{
    // Use accept4 with SOCK_NONBLOCK to avoid fcntl
    int client_fd = accept4(server_fd, NULL, NULL, SOCK_NONBLOCK);
    if (client_fd < 0)
    {
        std::cerr << "accept() failed: " << strerror(errno) << std::endl;
        return;
    }
    
    // Socket is already non-blocking (created with SOCK_NONBLOCK)
    
    Client client(client_fd, server_fd);
    client.setServerPtr(server);
    client.time = std::time(NULL);
    clients.insert(std::make_pair(client_fd, client));
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    ev.data.fd = client_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
    {
        close(client_fd);
        clients.erase(client_fd);
    }
}

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
        closeClient(epoll_fd, clients, toRemove[i]);
}

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

static void handleMultipart(Client &client, const Location &location)
{
    const std::string boundary = client.request.boundary;
    const std::string headerEnd = "\r\n\r\n";
    std::string &body = client.response.body;
    
    size_t boundPos;
    while ((boundPos = body.find(boundary)) != std::string::npos)
    {
        if (body.size() > boundPos + boundary.size() &&
            body[boundPos + boundary.size()] == '-')
        {
            client.response.multipartEnd = true;
            break;
        }
        
        size_t partStart = boundPos + boundary.size();
        if (partStart + 2 <= body.size() && body.substr(partStart, 2) == "\r\n")
            partStart += 2;
        
        size_t headersEnd = body.find(headerEnd, partStart);
        if (headersEnd == std::string::npos)
            break;
        
        std::string headers = body.substr(partStart, headersEnd - partStart);
        std::string filename = extractFilename(headers);
        if (filename.empty())
        {
            body = body.substr(headersEnd + headerEnd.size());
            continue;
        }
        
        size_t contentStart = headersEnd + headerEnd.size();
        size_t nextBoundary = body.find(boundary, contentStart);
        if (nextBoundary == std::string::npos)
            break;
        
        size_t contentEnd = nextBoundary;
        if (contentEnd >= 2 && body.substr(contentEnd - 2, 2) == "\r\n")
            contentEnd -= 2;
        
        std::string content = body.substr(contentStart, contentEnd - contentStart);
        
        std::string uploadPath = location.getUploadPath();
        if (uploadPath.empty())
            uploadPath = "./uploadFolder/";
        if (uploadPath[uploadPath.size() - 1] != '/')
            uploadPath += "/";
        
        std::string fullPath = uploadPath + filename;
        
        std::ofstream outFile(fullPath.c_str(), std::ios::binary | std::ios::app);
        if (outFile.is_open())
        {
            outFile.write(content.c_str(), content.size());
            outFile.close();
        }
        
        body = body.substr(nextBoundary);
    }
}

static void sendRedirect(Client &client, int code, const std::string &url)
{
    logRequest(code, client.request.methodName, client.request.rawPath);
    
    std::stringstream ss;
    ss << "HTTP/1.1 " << code << " Redirect\r\n";
    ss << "Location: " << url << "\r\n";
    ss << "Connection: close\r\n\r\n";
    
    std::string response = ss.str();
    send(client.client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    client.response.full = true;
}

static void sendCgiResponse(Client &client)
{
    logRequest(client.response.statusCode, client.request.methodName, client.request.rawPath);
    
    std::stringstream ss;
    ss << "HTTP/1.1 " << client.response.statusCode << " OK\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Content-Length: " << client.response.body.size() << "\r\n";
    ss << "Connection: close\r\n\r\n";
    ss << client.response.body;
    
    std::string response = ss.str();
    send(client.client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    client.response.full = true;
}

static void sendDirectoryListing(Client &client, Location &location)
{
    logRequest(200, client.request.methodName, client.request.rawPath);
    
    std::string content = indexof(location, client.request.path);
    send(client.client_fd, content.c_str(), content.size(), MSG_NOSIGNAL);
    client.response.full = true;
}

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
    
    if (location.getRedirectCode() != 0)
    {
        sendRedirect(client, location.getRedirectCode(), location.getRedirectUrl());
        return;
    }
    
    if (!location.isAllowedMethod(client.request.methodName))
    {
        client.response.statusCode = 405;
        client.error();
        return;
    }
    
    if (isCgiRequest(client.request.path, location) && !location.getCgiPath().empty())
    {
        // Start async CGI - will register pipes with epoll
        if (!startCgi(client, *client.server_ptr, epoll_fd))
        {
            // CGI failed to start
            client.error();
            return;
        }
        // CGI started - remove client from epoll until CGI completes
        // The CGI pipes are registered with epoll, checkCgiStatus will re-add client
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.client_fd, NULL);
        return;
    }
    
    if (mime(client.request.path) == FOLDER && client.request.methodName == "GET")
    {
        if (!location.getAutoIndex())
        {
            client.redirect(client.request.rawPath + location.getIndex());
            return;
        }
        
        sendDirectoryListing(client, location);
        return;
    }
    
    if (client.response.method == "DELETE" && client.response.statusCode == 200)
        client.response.deleteMethod(client.request.path);
    
    client.response.mimeType = client.response.getRightMimeType(client.request.path);
    
    if (client.response.statusCode != 200 && client.response.statusCode != 201)
    {
        client.error();
        return;
    }
    
    if (!client.response.file.is_open() && client.response.method != "DELETE")
        client.response.openFile(client.request.path);
    
    if (client.request.methodName == "POST")
    {
        if (!client.request.multipart)
            client.response.uploadBody();
        
        client.response.method = "GET";
        client.response.mimeType = "html";
        
        std::string responsePage = (client.response.statusCode == 201) 
            ? "./Post/201.html" : "./Post/200.html";
        client.request.path = responsePage;
        
        if (!client.response.file.is_open())
            client.response.openFile(client.request.path);
    }
    
    std::string response;
    if (!client.response.header)
    {
        response = client.response.createResponse(client.request);
        logRequest(client.response.statusCode, client.request.methodName, client.request.rawPath);
    }
    else
        response = client.response.rightMethod();
    
    ssize_t sent = send(client.client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    if (sent <= 0)
        client.response.full = true;
}

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

static int readPostBody(Client &client, int epoll_fd, std::map<int, Client> &clients)
{
    char buffer[READ_BUFFER_SIZE];
    ssize_t bytes_read = recv(client.client_fd, buffer, sizeof(buffer) , MSG_NOSIGNAL);
    
    if (bytes_read > 0)
    {
        client.response.body.append(buffer, bytes_read);
        client.request.body.append(buffer, bytes_read);
    }
    else if (bytes_read <= 0)
    {
        closeClient(epoll_fd, clients, client.client_fd);
        return -1;
    }
    
    if (client.response.body.size() >= client.response.contentLength)
    {
        client.request.full = true;
        client.request.body = client.response.body;
        return 0;
    }
    
    return 1;
}

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
    
    if (!client.request.fullHeader)
    {
        char buffer[READ_BUFFER_SIZE];
        ssize_t bytes_read = recv(client.client_fd, buffer, sizeof(buffer) , MSG_NOSIGNAL);
        
        if (bytes_read <= 0)
        {
            closeClient(epoll_fd, clients, client.client_fd);
            return;
        }
        
        client.entry.append(buffer, bytes_read);
        
        if (!parseRequestHeaders(client, server))
            return;
    }
    
    if ((client.response.statusCode == 200 || client.response.statusCode == 201) &&
        client.request.methodName == "POST" && !client.request.full)
    {
        int result = readPostBody(client, epoll_fd, clients);
        if (result == -1)
            return;
        
        if (client.request.multipart)
        {
            Location &location = server->getLocation(client.request.rawPath);
            handleMultipart(client, location);
            
            if (client.response.multipartEnd)
                client.request.full = true;
        }
        
        if (!client.request.full)
            return;
    }
    
    client.time = std::time(NULL);
    struct epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.fd = client.client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client.client_fd, &ev);
}

static void handleWriteEvent(int fd, int epoll_fd, std::map<int, Client> &clients,
                            struct epoll_event &events)
{
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client &client = it->second;
    
    // If CGI is active, wait for it to complete
    if (client.cgi.active)
        return;
    
    // If CGI completed, send CGI response
    if (client.cgi.done)
    {
        sendCgiResponse(client);
        if (client.response.full)
            closeClient(epoll_fd, clients, client.client_fd);
        return;
    }
    
    processResponse(clients, client, epoll_fd, events);
    
    if (client.response.full)
        closeClient(epoll_fd, clients, client.client_fd);
}

void multiple(std::vector<HttpServer> &servers)
{
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        std::cerr << "Failed to create epoll: " << strerror(errno) << std::endl;
        return;
    }
    
    std::map<int, Client> clients;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    
    try
    {
        for (std::vector<HttpServer>::iterator it = servers.begin();
             it != servers.end(); ++it)
        {
            it->setToEppoll(epoll_fd, ev);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        close(epoll_fd);
        return;
    }
    
    while (g_running)
    {
        checkTimeout(clients, epoll_fd);
        checkCgiStatus(clients, epoll_fd);
        
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT_MS);
        
        if (nfds == -1)
        {
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
            break;
        }
        
        if (nfds == 0)
            continue;
        
        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;
            uint32_t evts = events[i].events;
            
            HttpServer *server = findServerByFd(servers, fd);
            if (server != NULL)
            {
                acceptNewClient(fd, epoll_fd, clients, server);
                continue;
            }
            
            // Check if this is a CGI pipe fd
            std::map<int, int>::iterator cgiIt = g_cgiPipeToClient.find(fd);
            if (cgiIt != g_cgiPipeToClient.end())
            {
                int client_fd = cgiIt->second;
                std::map<int, Client>::iterator clientIt = clients.find(client_fd);
                if (clientIt != clients.end())
                {
                    Client &client = clientIt->second;
                    
                    if (evts & (EPOLLERR | EPOLLHUP))
                    {
                        // Read any remaining data before cleanup
                        if (client.cgi.pipeOut != -1 && fd == client.cgi.pipeOut)
                        {
                            char buffer[8192];
                            ssize_t bytes;
                            while ((bytes = read(client.cgi.pipeOut, buffer, sizeof(buffer))) > 0)
                            {
                                client.cgi.outputBuffer.append(buffer, bytes);
                            }
                        }
                        
                        // Parse output BEFORE cleanup (cleanup resets outputBuffer)
                        if (!client.cgi.outputBuffer.empty())
                        {
                            parseCgiOutput(client);
                        }
                        else
                        {
                            client.response.statusCode = 500;
                        }
                        
                        // Cleanup CGI
                        cleanupCgi(client, epoll_fd);
                        
                        // Mark CGI as done
                        client.cgi.done = true;
                        
                        // Re-add client to epoll to send response
                        struct epoll_event ev_tmp;
                        ev_tmp.events = EPOLLOUT;
                        ev_tmp.data.fd = client.client_fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client.client_fd, &ev_tmp);
                    }
                    else if (evts & EPOLLIN)
                    {
                        // CGI output ready to read
                        handleCgiRead(client, epoll_fd);
                    }
                    else if (evts & EPOLLOUT)
                    {
                        // CGI input ready to write
                        handleCgiWrite(client, epoll_fd);
                    }
                }
                continue;
            }
            
            if (evts & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                closeClient(epoll_fd, clients, fd);
                continue;
            }
            
            if (evts & EPOLLIN)
                handleReadEvent(fd, epoll_fd, clients, servers);
            else if (evts & EPOLLOUT)
                handleWriteEvent(fd, epoll_fd, clients, ev);
        }
    }
    
    std::cout << "\nCleaning up..." << std::endl;
    
    for (std::map<int, Client>::iterator it = clients.begin();
         it != clients.end(); ++it)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
        close(it->first);
    }
    clients.clear();
    
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
    
    close(epoll_fd);
    std::cout << "Server stopped." << std::endl;
}

