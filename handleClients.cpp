/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleClients.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/23 09:42:42 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/18 16:11:58 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "handleClients.hpp"
#include "Cgi.hpp"
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

HttpServer *findServer(std::vector<HttpServer> &servers, int server_fd)
{
    for (std::vector<HttpServer>::iterator it = servers.begin();
         it != servers.end();
         ++it)
    {
        if (it->getSocketFd() == server_fd)
            return &(*it);
    }
    return NULL;
}

void    acceptClient(int &fd, int &epoll_fd, std::map<int, Client> &clients, HttpServer* server , struct epoll_event &events)
{
    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0)
        return;
    
    // Set client socket to non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags == -1 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        close(client_fd);
        return;
    }
    
    Client client(client_fd, fd);
    client.setServerPtr(server);
    client.time = clock();
    clients.insert(std::make_pair(client_fd, client));
    events.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    events.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &events) == -1)
    {
        close(client_fd);
        clients.erase(client_fd);
        return;
    }
}

void    checkTimeout(std::map<int , Client> &clients, int epoll_fd)
{
    unsigned long   now;
    unsigned long   duration;
    std::vector<int> toRemove;
    std::map<int, Client>::iterator it;
    
    for (it = clients.begin(); it != clients.end(); ++it)
    {
        now = clock();
        duration = static_cast<unsigned long>(now - (it->second).time) / CLOCKS_PER_SEC;
        if (duration >= 60)
        {
            toRemove.push_back(it->first);
        }
    }
    
    for (size_t i = 0; i < toRemove.size(); i++)
    {
        int fd = toRemove[i];
        std::cout << "THE CLIENT " << fd << " TIMED OUT" << std::endl;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        clients.erase(fd);
    }
}

void    handleMultipart(Client &client)
{
    std::string delim1 = "\r\n";
    std::string delim2 = "\r\n\r\n";
    std::string toSearch = "filename=";
    size_t boundPos;
    // std::cout << "321" << client.response.body << "321" << std::endl;
    while ((boundPos = client.response.body.find(client.request.boundary)) != std::string::npos)
    {
        size_t pos = 0;
        size_t pos1 = 0;
        size_t pos2 = 0;
        size_t searchPos = 0;
        std::string filename;
        std::string content;
        std::string type;

        if ((searchPos = client.response.body.find(toSearch)) != std::string::npos)
        {
            if (searchPos > boundPos)
            {
                client.response.body = client.response.body.substr(boundPos + client.request.boundary.size() + delim1.size());
                searchPos = client.response.body.find(toSearch);
                if ((boundPos = client.response.body.find(client.request.boundary)) == std::string::npos)
                    break;
            }
            filename = client.response.body.substr(searchPos + toSearch.size());
            pos1 = filename.find(delim1);
            filename = filename.substr(0, pos1);
            if (filename[0] == '"' && filename[filename.length() - 1] == '"')
                filename = filename.substr(1, filename.length() - 2);
            // std::ofstream multipartFile;
            std::cout << "PATH: " << filename << "+++" << std::endl;

            type = client.response.getRightMimeType(filename);
            std::cout << "TYPE: " << type << std::endl;
            std::ofstream multipartFile;
            filename = "./uploadFolder/" + filename;
            if (type == "png" || type == "jpeg" || type == "jpg")
            {
                multipartFile.open(filename.c_str(), std::ios::binary | std::ios::app);
            }
            else
            {
                multipartFile.open(filename.c_str(), std::ios::app);
            }
            // multipartFile(filename.c_str());
            pos2 = client.response.body.find(delim2);
            content = client.response.body.substr(pos2 + delim2.size());
            pos = content.find(client.request.boundary);
            content = content.substr(0, pos);
            multipartFile.write(content.c_str(), content.size());
            // multipartFile << content;
            multipartFile.close();
        }
        if (client.response.body[boundPos + client.request.boundary.size()] == '-')
        {
            client.response.multipartEnd = true;
        }
        // multipartFile.close();
        std::cout << "boundary: " << client.request.boundary << " | " << client.request.boundary.size() << std::endl;
        client.response.body = client.response.body.substr(boundPos + client.request.boundary.size());
        std::cout << "\e[1;32m" << client.response.body << "\e[0m" << std::endl;
    }
}

void    setupResponse(std::map<int, Client> &clients, int &fd, Client &client, int &epoll_fd, struct epoll_event &events)
{
    (void)(clients);
    (void)(fd);

    // Handle redirects first
    Location &location = client.server_ptr->getLocation(client.request.rawPath);
    if (location.getRedirectCode() != 0)
    {
        std::string res = "HTTP/1.0 " + intToString(location.getRedirectCode()) + " Redirect\r\n";
        res += "Location: " + location.getRedirectUrl() + "\r\n\r\n";
        send(client.client_fd, res.c_str(), res.size(), MSG_NOSIGNAL);
        client.response.full = true;
        return;
    }

    // Check if method is allowed for this location
    if (!location.isAllowedMethod(client.request.methodName))
    {
        client.response.statusCode = 405;
        client.error();
        return;
    }

    // Check for CGI request
    bool isCgi = isCgiRequest(client.request.path, location);
    
    if (isCgi && !location.getCgiPath().empty())
    {
        handleCgi(client, *client.server_ptr, epoll_fd, events);
        
        if (client.response.statusCode != 200)
        {
            client.error();
            return;
        }
        
        // Send CGI response
        std::stringstream response;
        response << "HTTP/1.0 200 OK\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << client.response.body.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << client.response.body;
        
        std::string res = response.str();
        send(client.client_fd, res.c_str(), res.size(), MSG_NOSIGNAL);
        client.response.full = true;
        return;
    }

    if(mime(client.request.path) == FOLDER && client.request.methodName == "GET")
    {
        if(!location.getAutoIndex())
        {
            client.redirect(client.request.rawPath + location.getIndex());
            return;
        }

        std::string indexContent = indexof(location, client.request.path);
        send(client.client_fd, indexContent.c_str(), indexContent.size() , MSG_NOSIGNAL);
        client.response.full = true;
        return;
    }

    // std::cout << "\e[1;31mClient: " << client.request.path << "\e[0m" << std::endl;

    if ((client.response.method == "DELETE") && (client.response.statusCode == 200))
        client.response.deleteMethod(client.request.path);
    client.response.mimeType = client.response.getRightMimeType(client.request.path);
    if (client.response.statusCode != 200 && client.response.statusCode != 201)
    {
        client.error();
        return;
    }
    if ((!(client.response.file.is_open())) && (client.response.method != "DELETE"))
        client.response.openFile(client.request.path);
    std::string response;
    if (client.request.methodName == "POST")
    {
        std::cout << "8" << client.response.body << "8" << std::endl;
        if (!client.request.multipart)
            client.response.uploadBody();
        client.response.method = "GET";
        client.response.mimeType = "html";
        if (client.response.statusCode == 201)
            client.request.path = "./Post/201.html";
        else if (client.response.statusCode == 200)
            client.request.path = "./Post/200.html";
        if ((!(client.response.file.is_open())) && (client.response.method != "DELETE"))
            client.response.openFile(client.request.path);
    }
    if (!(client.response.header))
        response = client.response.createResponse(client.request);
    else
        response = client.response.rightMethod();
    ssize_t size = send(client.client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    if(size <= 0)
    {
        close(client.client_fd);
        clients.erase(client.client_fd);
    }
}

void multiple(std::vector<HttpServer> &servers)
{
    int epoll_fd = epoll_create(1);
    struct epoll_event events, all_events[10];
    std::map<int, Client> clients;
    std::vector<HttpServer>::iterator it;
    std::string uploadFolder = "/uploadFolder";

    if (epoll_fd == -1)
    {
        std::cerr << "Failed to create epoll instance." << std::endl;
        return;
    }    
    for (it = servers.begin(); it != servers.end(); ++it)
    {
        (*it).setToEppoll(epoll_fd, events);
    }

    while (true)
    {
        // Check for client timeouts periodically
        checkTimeout(clients, epoll_fd);
        
        int count = epoll_wait(epoll_fd, all_events, 10, 1000); // 1 second timeout
        if (count == -1)
        {
            if (errno == EINTR)
                continue;
            std::cerr << "epoll wait error" << std::endl;
            return;
        }
        else if (count == 0)
        {
            // Timeout, loop back to check for client timeouts
            continue;
        }
        else
        {
            std::cerr << "\e[90m-------------------------------------------------------- \e[0m" << std::endl;
            for (int i = 0; i < count; i++)
            {
                int fd = all_events[i].data.fd;
                HttpServer *server = findServer(servers, fd);
                if (server != NULL)
                    acceptClient(fd, epoll_fd, clients , server , events);
                else
                {
                    if ((all_events[i].events & EPOLLIN))
                    {
                        if (clients.find(fd) == clients.end())
                            continue;
                        Client &client = clients.find(fd)->second;
                        HttpServer *serverPtr = findServer(servers, client.server_fd);
                        if (!serverPtr)
                        {
                            client.response.statusCode = 500;
                            events.events = EPOLLOUT;
                            events.data.fd = client.client_fd;
                            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client.client_fd, &events);
                            continue;
                        }
                        std::vector<char> buffer(4096);
                        if (!(client.request.fullHeader))
                        {
                            int bytes_read = read(client.client_fd, buffer.data(), buffer.size());

                            if(bytes_read <= 0)
                            {
                                    close(client.client_fd);
                                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.client_fd, NULL);
                                    clients.erase(client.client_fd);
                                    continue;
                            }

                            std::string entry(buffer.begin(), buffer.begin() + bytes_read);
                            client.entry += entry;
                            
                            if (client.entry.find("\r\n\r\n") != std::string::npos)
                            {
                                client.checkHead();
                                Location location;
                                location = serverPtr->getLocation(client.request.path);
                                std::string root = location.getRoot();
                                client.request.rawPath = client.request.path;
                                client.setServerPtr(client.server_ptr);
                                client.checkPath();
                                client.request.fullHeader = true;

                                std::cout << client.request.methodName << " " << client.request.path << std::endl; 
                            }
                            else
                                continue;
                        }
                        if ((client.response.statusCode == 200 || client.response.statusCode == 201) && client.request.methodName == "POST")
                        {
                            if (client.request.multipart || (!client.request.full))
                            {
                                if (!client.request.full)
                                {
                                    int bytes_read = read(client.client_fd, buffer.data(), buffer.size());
                                    if (bytes_read > 0)
                                    {
                                        client.response.body.append(buffer.data(), bytes_read);
                                        client.request.body.append(buffer.data(), bytes_read);
                                    }
                                    else if (bytes_read == 0)
                                    {
                                        // Connection closed
                                        close(client.client_fd);
                                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.client_fd, NULL);
                                        clients.erase(client.client_fd);
                                        continue;
                                    }
                                    else if (errno != EAGAIN && errno != EWOULDBLOCK)
                                    {
                                        // Read error
                                        close(client.client_fd);
                                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.client_fd, NULL);
                                        clients.erase(client.client_fd);
                                        continue;
                                    }
                                }
                                if (client.request.multipart)
                                    handleMultipart(client);
                                if (client.response.body.size() >= client.response.contentLength || client.response.multipartEnd)
                                {
                                    client.request.full = true;
                                    client.request.body = client.response.body;
                                }
                                else
                                    continue;
                            }
                        }
                        client.time = clock();
                        events.events = EPOLLOUT;
                        events.data.fd = client.client_fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client.client_fd, &events);
                    }
                    if ((all_events[i].events & EPOLLOUT))
                    {
                        if (clients.find(fd) == clients.end())
                            continue;
                        Client &client = clients.find(fd)->second;
                        setupResponse(clients, fd, client, epoll_fd, events);      
                        if (!(client.response.full))
                            continue;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.client_fd, NULL);
                        close(client.client_fd);
                        clients.erase(client.client_fd);
                    }
                    if ((all_events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)))
                    {
                        if (clients.find(fd) == clients.end())
                            continue;
                        Client &client = clients.find(fd)->second;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.client_fd, NULL);
                        close(client.client_fd);                       
                        clients.erase(client.client_fd);
                    }
                }
            }
        }
    }
}

