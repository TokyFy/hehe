/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sranaivo <sranaivo@student.42antananarivo. +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 14:47:51 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/18 21:28:31 by sranaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "HttpServer.hpp"
#include "utils.hpp"
#include <string>
#include <cstdlib>

Client::Client(void)
{
    sent = 0;
    server_ptr = NULL;
}

void    Client::assignFds(int fd, int server)
{
    server_fd = server;
    client_fd = fd;
    server_ptr = NULL;
}

Client::~Client(void)
{

}

Client::Client(int fd, int server): server_fd(server), server_ptr(NULL), client_fd(fd), sent(0)
{

}

void    Client::set(void)
{
}

int     Client::checkHead(void)
{
    std::string head;
    int         code;

    response.statusCode = 200;
    size_t pos = entry.find("\r");
    head = entry.substr(0, pos);
    code = request.parseHead(head, response);
    if (code == 400)
        response.statusCode = 400; 
    else if (code)
        response.statusCode = 405;
    request.parse(entry);
    if (request.methodName == "POST" && response.statusCode == 200)
    {
        std::string length = request.getPair("Content-Length");
        if (length.empty())
        {
            std::cerr << "\033[31m[REJECTED]\033[0m POST request missing Content-Length header" << std::endl;
            response.statusCode = 411;
        }
        else
            response.contentLength = std::atoi(length.c_str());
        
        if (server_ptr && static_cast<ssize_t>(response.contentLength) > server_ptr->getClientMaxBodySize())
        {
            std::cerr << "\033[31m[REJECTED]\033[0m Request body too large: " 
                      << response.contentLength << " bytes (max: " 
                      << server_ptr->getClientMaxBodySize() << " bytes)" << std::endl;
            response.statusCode = 413;
        }
    }
    request.checkMultipart();
    pos = entry.find("\r\n\r\n");
    if ((pos + 4))
    {
        response.body = entry.substr(pos + 4);
        // Don't mark as full for multipart - let streaming handle it
        if (!request.multipart && response.body.size() == response.contentLength)
            request.full = true;
    }
    set();
    return (0);
}

Client &Client::operator=(Client const &to_what)
{
    if (this != &to_what)
    {
        server_fd = to_what.server_fd;
        client_fd = to_what.client_fd;
        sent = to_what.sent;
        entry = to_what.entry;
        time = to_what.time;
        request = to_what.request;
        response = to_what.response;
        server_ptr = to_what.server_ptr;
        cgi = to_what.cgi;
    }
    return (*this);
}

Client::Client(Client const &to_copy)
{
    *this = to_copy;
}

void    Client::reset(void)
{
    sent = 0;
    entry.clear();
    request = HttpRequest();
    response = HttpResponse();
}

int Client::checkPath(void)
{
    std::string     path = request.path;
    std::ifstream   file(path.c_str());

    if (response.statusCode == 400 || response.statusCode == 405)
        return (0);
    if (!file.is_open())
    {
        if (access(path.c_str(), F_OK))
        {
            if (request.methodName == "POST")
            {
                response.statusCode = 201;
                return (1);
            }
            response.statusCode = 404;
            return (0);
        }
        if (access(path.c_str(), R_OK))
        {
            response.statusCode = 403;
            return (0);
        }
        response.statusCode = 500;
        return (0);
    }
    file.close();
    return (1);
}

void Client::setServerPtr(HttpServer* ptr)
{
    if(!ptr)
        return;

    server_ptr = ptr;

    std::string rpath = ptr->getRoutedPath(request.path);
    if (rpath.size() > 0 && rpath[0] == '/')
        request.path = "." + rpath;
    else if (rpath.size() >= 2 && rpath[0] == '.' && rpath[1] == '/')
        request.path = rpath;
    else
        request.path = "./" + rpath;
}

void Client::redirect(const std::string& path)
{
    std::string res = "HTTP/1.0 302 Found\r\nLocation: " + path +"\r\n\r\n";
    send(client_fd, res.c_str(), res.size() , MSG_NOSIGNAL);
    response.full = true;
}

void Client::sendStatusPage()
{
    std::string pagePath = server_ptr->getErrorPage(response.statusCode);
    std::string html;

    // Try to read custom page from config
    if(pagePath.size() > 0 && access(pagePath.c_str(), F_OK) == 0)
    {
        std::ifstream file(pagePath.c_str());
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            html = buffer.str();
            file.close();
        }
    }
    
    // Fallback to default page
    if (html.empty())
    {
        std::stringstream defaultHtml;
        defaultHtml << "<html><head>";
        defaultHtml << "<title>" << response.statusCode << " " << response.getStatusMessage() << "</title>";
        defaultHtml << "<style> body { font-family: monospace;} </style>";
        defaultHtml << "</head><body>";
        defaultHtml << "<img src=\"/favicon.ico\"><br/><br/>";
        defaultHtml << "<b>" << itos(response.statusCode) << " - " << response.getStatusMessage() << "</b>";
        if (response.statusCode >= 400)
            defaultHtml << "<p>An error occurred.</p>";
        else if (response.statusCode == 201)
            defaultHtml << "<p>Resource created successfully.</p>";
        else if (response.statusCode == 200)
            defaultHtml << "<p>Request completed successfully.</p>";
        defaultHtml << "</body></html>";
        html = defaultHtml.str();
    }

    std::stringstream resp;
    
    resp << "HTTP/1.1 " << itos(response.statusCode) << " " << response.getStatusMessage() << "\r\n";
    resp << "Content-Type: text/html\r\n";
    resp << "Content-Length: " << html.size() << "\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << html;
   
    std::string res = resp.str();
    send(client_fd, res.c_str(), res.size() , MSG_NOSIGNAL);
    response.full = true;     

    return;
}

void Client::error()
{
    sendStatusPage();
}
