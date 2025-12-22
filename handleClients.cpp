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
    Client client(client_fd, fd);
    // Associer le serveur au client avant l'insertion pour éviter un pointeur nul dans la map
    client.server_ptr = server;
    clients.insert(std::make_pair(client_fd, client));
    events.events = EPOLLIN;
    events.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &events))
    {
        std::cerr << "Failed to add fd." << std::endl;
        return;
    }
    else
        std::cout << "added a new client with fd " << client_fd << std::endl;
}

void    checkTimeout(std::map<int , Client> &clients, int epoll_fd)
{
    unsigned long   now;
    unsigned long   duration;
    std::map<int, Client>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it)
    {
        now = clock();
        duration = static_cast<unsigned long>(now - (it->second).time) / CLOCKS_PER_SEC;
        if (duration >= 100)
        {
            int fd = (it->second).client_fd;
            std::cout << "THE CLIENT " << fd << " TIMED OUT" << std::endl;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            clients.erase(fd);
        }
    }
}

void    handleMultipart(Client &client)
{
    std::cout << "\e[1;36mMULTI\e[0m" << std::endl;

    std::string delim1 = "\r\n";
    std::string delim2 = "\r\n\r\n";
    std::string toSearch = "filename=";
    size_t boundPos;
    // std::cout << "321" << client.response.body << "321" << std::endl;
    while ((boundPos = client.response.body.find(client.request.boundary)) != std::string::npos)
    {
        std::cout << "\e[1;36mMULTI2\e[0m" << std::endl;

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
                std::cout << "\e[1;36mMANALA ANLE VOALOHANY\e[0m" << std::endl;
                client.response.body = client.response.body.substr(boundPos + client.request.boundary.size() + delim1.size());
                std::cout << "+++" << client.response.body << "+++"  << std::endl;
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
                std::cout << "TSY BINARY" << std::endl;
            }
            else
            {
                std::cout << "\e[1;32mBINARY\e[0m" << std::endl;
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

void    setupResponse(std::map<int, Client> &clients, int &fd, Client &client)
{
    (void)(clients);
    (void)(fd);

    std::cout << "INDEX OF ** " << client.request.path << " * " <<client.server_ptr->getRoutedPath("/public/foo") << "\e[0m" << std::endl;

    if(mime(client.request.path) == FOLDER)
    {
        std::string indexContent = "";
        if (client.server_ptr)
        {
            Location location = client.server_ptr->getLocation(client.request.path);
            indexContent = indexof(location, client.request.path);
        }
        else
             std::string res = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nError";


        std::cout << "INDEX CONTENT: " << indexContent << std::endl;
        send(client.client_fd, indexContent.c_str(), indexContent.size() , MSG_NOSIGNAL);
        client.response.full = true;
        return;
    }

    std::cout << "\e[1;31mClient: " << client.request.path << "\e[0m" << std::endl;
    // client.request.parse(client.entry);
    if ((client.response.method == "DELETE") && (client.response.statusCode == 200))
        client.response.deleteMethod(client.request.path);
    client.response.mimeType = client.response.getRightMimeType(client.request.path);
    if (client.response.statusCode != 200 && client.response.statusCode != 201)
    {
        std::cout << "ERROR PAGE" << std::endl;
        client.response.method = "GET";
        client.request.path = "./www/error.html";
        client.response.mimeType = "html";
    }
    if ((!(client.response.file.is_open())) && (client.response.method != "DELETE"))
        client.response.openFile(client.request.path);
    std::cout << "ETO" << std::endl;
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
    (void)(size);
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
        std::cout << "HEEEEEEEEEEEEEEEEEEEEEEEEERRRRRRRRRRRRRRRRRRRRRRREEEEEEEEEE" << std::endl;
        (*it).setToEppoll(epoll_fd, events);
    }

    while (true)
    {
        int count = epoll_wait(epoll_fd, all_events, 10, -1);
        if (count == -1)
        {
            std::cerr << "epoll wait error" << std::endl;
            return;
        }
        else
        {
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
                            std::string entry(buffer.begin(), buffer.begin() + bytes_read);
                            client.entry += entry;
                            // std::cout << "---" << client.entry << "---" << std::endl;
                            if (client.entry.find("\r\n\r\n") != std::string::npos)
                            {
                                client.checkHead();
                                Location location;
                                location = serverPtr->getLocation(client.request.path);
                                std::string root = location.getRoot();
                                if (client.request.methodName != "POST")
                                    client.request.path = root + client.request.path;
                                else
                                    client.request.path = root + uploadFolder + client.request.path;
                                client.checkPath();
                                // std::cout << "2" << client.response.body << "2" << std::endl;
                                client.request.fullHeader = true;
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
                                    if (bytes_read <= 0)
                                        std::cout << "HEHEHEHEHHEHEHE" << std::endl;
                                    client.response.body.append(buffer.data(), bytes_read);
                                }
                                if (client.request.multipart)
                                    handleMultipart(client);
                                if (client.response.body.size() == client.response.contentLength || client.response.multipartEnd)
                                    client.request.full = true;
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
                        std::cout << "epollout event occured client to answer : " << fd << std::endl;
                        if (clients.find(fd) == clients.end())
                            continue;
                        Client &client = clients.find(fd)->second;
                        setupResponse(clients, fd, client);      
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
                        std::cout << "something else occured" << std::endl;
                    }
                    std::cout << "\n";
                }
            }
            std::cerr << "\e[1;33mClient Number: \e[0m" << clients.size() << std::endl;
        }
    }
}
/* 
void    multiple(int server_fd)
{
    std::string root = ".";
    std::string uploadFolder = "/uploadFolder";
    std::cout << "server fd : " << server_fd << std::endl;
    struct epoll_event events, all_events[10];
    std::map<int, Client> clients;
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        std::cerr << "Failed to create epoll instance." << std::endl;
        return;
    }
    events.events = EPOLLIN;
    events.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &events))
    {
        std::cerr << "Failed to add fd." << std::endl;
        return;
    }
    while (true)
    {
        int count = epoll_wait(epoll_fd, all_events, 10, -1);
        if (count == -1)
        {
            std::cout << "epoll wait error" << std::endl;
            return;
        }
        else if (count == 0)
        {
            std::cout << "waited too long" << std::endl;
            return;
        }
        else
        {
            std::cout << "BLABLA: " << count << std::endl;
            for (int i = 0; i < count; i++)
            {
                // checkTimeout(clients, epoll_fd);
                if (all_events[i].data.fd == server_fd)
                {
                    int client_fd = accept(server_fd, NULL, NULL);
                    std::cout << "\e[1;31mClient: " << client_fd << "\e[0m" << std::endl;
                    Client client(client_fd, server_fd);
                    clients.insert(std::make_pair(client_fd, client));
                    // clients.insert({client_fd, client});
                    std::cout << "entry at the start: " << client.entry << std::endl;
                    events.events = EPOLLIN;
                    events.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &events))
                    {
                        std::cerr << "Failed to add fd." << std::endl;
                        return;
                    }
                    else
                        std::cout << "added a new client with fd " << client_fd << std::endl;
                }
                else
                {
                    int fd = all_events[i].data.fd;
                    if ((all_events[i].events & EPOLLIN))
                    {
                        if (clients.find(fd) == clients.end())
                            break;
                        Client &client = clients.find(fd)->second;
                        std::vector<char> buffer(4096);
                        // client.entry.clear();
                        std::cout << "*" << fd << "*" << std::endl;
                        std::cout << "epollin client : " << client.client_fd << " | " << fd << std::endl;
                        if (!client.request.fullHeader)
                        {
                            int bytes_read = read(client.client_fd, buffer.data(), buffer.size());
                            std::string entry(buffer.begin(), buffer.begin() + bytes_read);
                            client.entry += entry;
                            // std::cout << "---" << client.entry << "---" << std::endl;
                            if (client.entry.find("\r\n\r\n") != std::string::npos)
                            {
                                client.checkHead();
                                if (client.request.methodName != "POST")
                                    client.request.path = root + client.request.path;
                                else
                                    client.request.path = root + uploadFolder + client.request.path;
                                client.checkPath();
                                // std::cout << "2" << client.response.body << "2" << std::endl;
                                client.request.fullHeader = true;
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
                                    std::cout << "\e[1;36m TSY FFEEEEEEEEEEENNNNNNNNNNNOOOOOOOOO\e[0m" << std::endl;

                                    // size_t mirado;
                                    // if (client.response.contentLength < buffer.size())
                                    //     mirado = client.response.contentLength;
                                    // else
                                    //     mirado = buffer.size();
                                    int bytes_read = read(client.client_fd, buffer.data(), buffer.size());
                                    if (bytes_read <= 0)
                                        std::cout << "HEHEHEHEHHEHEHE" << std::endl;
                                    std::cout << "THE DATA: " << buffer.data() << ".END" << std::endl;
                                    client.response.body.append(buffer.data(), bytes_read);
                                    std::cout << "\e[1;31m" << client.response.body << "\e[0m" << std::endl;
                                }
                                if (client.request.multipart)
                                {
                                    std::cout << "\e[1;36mMULTI\e[0m" << std::endl;

                                    std::string delim1 = "\r\n";
                                    std::string delim2 = "\r\n\r\n";
                                    std::string toSearch = "filename=";
                                    size_t boundPos;
                                    // std::cout << "321" << client.response.body << "321" << std::endl;
                                    while ((boundPos = client.response.body.find(client.request.boundary)) != std::string::npos)
                                    {
                                        std::cout << "\e[1;36mMULTI2\e[0m" << std::endl;

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
                                                std::cout << "\e[1;36mMANALA ANLE VOALOHANY\e[0m" << std::endl;
                                                client.response.body = client.response.body.substr(boundPos + client.request.boundary.size() + delim1.size());
                                                std::cout << "+++" << client.response.body << "+++"  << std::endl;
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
                                            if (type == "png" || type == "jpeg" || type == "jpg")
                                            {
                                                multipartFile.open(filename.c_str(), std::ios::binary | std::ios::app);
                                                std::cout << "TSY BINARY" << std::endl;
                                            }
                                            else
                                            {
                                                std::cout << "\e[1;32mBINARY\e[0m" << std::endl;
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
                                // std::string entry(buffer.begin(), buffer.begin() + bytes_read);
                                // client.response.body += entry;
                                if (client.response.body.size() == client.response.contentLength || client.response.multipartEnd)
                                {
                                    std::cout << "\e[1;36m FFEEEEEEEEEEENNNNNNNNNNNOOOOOOOOO\e[0m" << std::endl;
                                    client.request.full = true;
                                }
                                else
                                    continue;
                            }
                        }
                        client.time = clock();
                        // std::cout << "THE REQUEST : *" << client.entry << "*" << std::endl;
                        events.events = EPOLLOUT;
                        events.data.fd = client.client_fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client.client_fd, &events);
                    }
                    if ((all_events[i].events & EPOLLOUT))
                    {
                        std::cout << "epollout event occured client to answer : " << fd << std::endl;
                        if (clients.find(fd) == clients.end())
                            break;
                        Client &client = clients.find(fd)->second;
                        std::cout << "\e[1;31mClient: " << client.request.path << "\e[0m" << std::endl;
                        // client.request.parse(client.entry);
                        if ((client.response.method == "DELETE") && (client.response.statusCode == 200))
                            client.response.deleteMethod(client.request.path);
                        client.response.mimeType = client.response.getRightMimeType(client.request.path);
                        if (client.response.statusCode != 200 && client.response.statusCode != 201)
                        {
                            std::cout << "ERROR PAGE" << std::endl;
                            client.response.method = "GET";
                            client.request.path = "./www/error.html";
                            client.response.mimeType = "html";
                        }
                        if ((!(client.response.file.is_open())) && (client.response.method != "DELETE"))
                            client.response.openFile(client.request.path);
                        std::cout << "ETO" << std::endl;
                        std::string response;
                        if (client.request.methodName == "POST")
                        {
                            std::cout << "8" << client.response.body << "8" << std::endl;
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
                        if (size == 0)
                            continue;                          
                        if (!(client.response.full))
                            continue;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.client_fd, NULL);
                        close(client.client_fd);
                        clients.erase(client.client_fd);
                    }
                    if ((all_events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)))
                    {
                        if (clients.find(fd) == clients.end())
                            break;
                        Client &client = clients.find(fd)->second;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.client_fd, NULL);
                        close(client.client_fd);                       
                        clients.erase(client.client_fd);
                        std::cout << "something else occured" << std::endl;
                    }
                    else
                    {
                        std::cout << "nah nah nah" << std::endl;
                    }
                    std::cout << "\n";
                }
            }
            std::cerr << "\e[1;33mClient Number: \e[0m" << clients.size() << std::endl;
        }
    }
} */
