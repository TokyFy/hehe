/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: franaivo <franaivo@student.42antananarivo  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 09:01:24 by franaivo          #+#    #+#             */
/*   Updated: 2025/11/06 09:03:46 by franaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpServer.hpp"
#include "HttpAgent.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <string>
#include <strings.h>
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <cerrno>
#include <netinet/in.h>
#include <iostream>


HttpServer::HttpServer(int socket_fd)
    : HttpAgent(socket_fd, SERVER) , name("~ SERVER ~") , client_max_body_size(1024) , port(-1) ,
        interface("0.0.0.0")
{
    return;
}

HttpServer::HttpServer(int socket_fd , std::string name)
    : HttpAgent(socket_fd, SERVER) , name(name) , client_max_body_size(1024) , port(-1) ,
        interface("0.0.0.0")
{
    return;
}

HttpServer::~HttpServer()
{
    if(socket_fd > 0)
        close(socket_fd);
    return;
}

ssize_t HttpServer::getClientMaxBodySize() const
{
    return client_max_body_size;
}
        
void    HttpServer::setClientMaxBodySize(ssize_t value)
{
    client_max_body_size = value;
}

int     HttpServer::getPort() const
{
    return port;
}

void    HttpServer::setPort(int value)
{
    port = value;
}

const std::string&  HttpServer::getInterface() const
{
    return interface;
}

void    HttpServer::setInterface(const std::string& value)
{
    interface = value;
}

const std::string&  HttpServer::getName() const
{
    return name;
}

void    HttpServer::setName(const std::string& value)
{
    name = value;
}

void HttpServer::setErrorPage(int code , std::string path)
{
    if(code <= 0 || code >= 0x255)
        throw std::runtime_error("Invalid error code");
    
    error_pages[code] = path; 
}

const std::string& HttpServer::getErrorPage(int code) {

    (void)(code);

    std::map<int,std::string>::const_iterator it = error_pages.find(code);
    if (it == error_pages.end())
        throw std::runtime_error("Error page not found");

    return it->second;
}


void HttpServer::addLocation(Location& location)
{
    locations.push_back(location);
}

void HttpServer::setToEppoll(int epoll_fd, struct epoll_event &events)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        throw std::runtime_error(std::string("socket failed: ") + strerror(errno));

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        close(server_fd);
        throw std::runtime_error(std::string("setsockopt failed: ") + strerror(errno));
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        close(server_fd);
        throw std::runtime_error(std::string("bind failed: ") + strerror(errno));
    }

    if (listen(server_fd, 10) == -1)
    {
        close(server_fd);
        throw std::runtime_error(std::string("listen failed: ") + strerror(errno));
    }
    events.events = EPOLLIN;
    events.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &events) == -1)
    {
        close(server_fd);
        throw std::runtime_error(std::string("epoll_ctl failed: ") + strerror(errno));
    }
    socket_fd = server_fd;
}

bool starts_with(const std::string& str, const std::string& prefix)
{
    if (str.size() < prefix.size())
        return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

Location& HttpServer::getLocation(std::string& path)
{
    std::vector<Location>::iterator it = locations.begin();

    while(it != locations.end())
    {
        if(starts_with(path , it->getSource()))
            return *it;
        it++;
    }

    return locations[locations.size() - 1];
}

bool matchLocation(const std::string& path, const std::string& src)
{
    if (!starts_with(path, src))
        return false;

    return path[src.size()] == '/';
}

std::string HttpServer::getRoutedPath(const std::string& requestedPath) const
{
    if (locations.empty())
        throw std::runtime_error("Server should have at least 1 location");

    // Default to LAST location
    std::vector<Location>::const_iterator matched = locations.end();
    --matched;

    // Longest prefix match
    for (std::vector<Location>::const_iterator it = locations.begin();
         it != locations.end(); ++it)
    {
        const std::string& src = it->getSource();

        if (starts_with(requestedPath, src))
        {
            matched = it;
            break;
        }
    }

    const std::string& src = matched->getSource();
    std::string suffix;

    if (requestedPath.size() >= src.size())
        suffix = requestedPath.substr(src.size());

    std::string root = matched->getRoot();

    // Avoid double '/'
    if (!root.empty() && root[root.size() - 1] == '/' &&
        !suffix.empty() && suffix[0] == '/')
    {
        root.erase(root.size() - 1);
    }

    return root + suffix;
}


void HttpServer::normalize()
{
    if(port == -1)
        throw std::runtime_error("Server should have an Interface:port");
    
    if(locations.empty())
        throw std::runtime_error("Server should have at least 1 location");

    std::vector<Location>::iterator it = locations.begin();

    while(it != locations.end())
    {
        it->normalize();
        it++;
    }
}

// Location Object

Location::Location()
    : source("/") , autoindex(true) , root("/") , allow_methods(0b000)
{
    return;
}

Location::~Location()
{
    (void)(allow_methods);
    return;
}

const std::string& Location::getSource() const {
    return source;
}

void Location::setSource(const std::string& value) {
    source = value;
}

const std::string& Location::getIndex() const {
    return index;
}

void Location::setIndex(const std::string& value) {
    index = value;
}

bool Location::getAutoIndex() const {
    if(index.size() == 0)
        return false;

    return autoindex;
}

void Location::setAutoIndex(bool value) {
    autoindex = value;
}

const std::string& Location::getRoot() const {
    return root;
}

void Location::setRoot(const std::string& value) {
    root = value;
}

const std::string& Location::getCgiPath() const {
    return cgi_path;
}

void Location::setCgiPath(const std::string& value) {
    cgi_path = value;
}

const std::string& Location::getCgiExt() const {
    return cgi_ext;
}

void Location::setCgiExt(const std::string& value) {
    cgi_ext = value;
}


const std::string&  Location::getUploadPath() const
{
    return upload;
}

void                Location::setUploadPath(const std::string& path)
{
    upload = path;
}

void Location::addAllowedMethod(const std::string & method)
{
    if(method == "GET")
        allow_methods |= (1 << 0);
    else if(method == "POST")
        allow_methods |= (1 << 1);
    else if(method == "DELETE")
        allow_methods |= (1 << 2);
    else 
        throw std::runtime_error("Unkown method " + method);
}

bool Location::isAllowedMethod(const std::string& method)
{
    if(method == "GET")
        return ( allow_methods >> 0 ) & 1;
    else if(method == "POST")
        return ( allow_methods >> 1 ) & 1;
    else if(method == "DELETE")
        return ( allow_methods >> 2 ) & 1;
    else 
        return false;
}

void    Location::normalize()
{
    if(root.empty())
        throw std::runtime_error("Location should have a route");

    if(index.empty() && !autoindex)
        throw std::runtime_error("Index needed when autoindex is false");
   
    if(allow_methods == 0)
        throw std::runtime_error("Location should have at least on method allowed");

    if(upload.empty() && !this->isAllowedMethod("POST"))
        throw  std::runtime_error("Location should have upload path when POST allowed");
}


