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
    // std::cout << "Client entry : " << entry << std::endl;
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
            response.statusCode = 411;
        else
            response.contentLength = std::atoi(length.c_str());
        
        if (server_ptr && static_cast<ssize_t>(response.contentLength) > server_ptr->getClientMaxBodySize())
        {
            response.statusCode = 413;
        }
    }
    pos = entry.find("\r\n\r\n");
    if ((pos + 4))
    {
        response.body = entry.substr(pos + 4);
        // std::cout << "1" << response.body << "1" << std::endl;
        if (response.body.size() == response.contentLength)
            request.full = true;
    }
    request.checkMultipart();
    set();
    return (0);
    //     return (1);
    // request.parseHeaders(entry.substr(pos + 2));
    // if (crochet != std::string::npos)
    //     request.body = entry.substr(crochet);
    // return (0);

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
    }
    return (*this);
}

Client::Client(Client const &to_copy)
{
    *this = to_copy;
}

void    Client::reset(void)
{
    std::cout << "resetting" << std::endl;
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
    // Only add "." if path doesn't already start with "./" or "/"
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

void Client::error()
{
    std::string errorPath = server_ptr->getErrorPage(response.statusCode);

    if(errorPath.size() > 0)
    {
        redirect(errorPath);
        return;
    }
    
    std::stringstream html;

    html << "<html><head>";
    html << "<title>Error</title>";
    html << "<style> body { font-family: monospace; line-height: 1.2em; } </style>";
    html << "</head><body>";
    html << "<h4>ERROR "<< itos(response.statusCode) << "</h4><hr>" << "<p>" + response.getStatusMessage() + "</p>";
    html << "</body></html>";

    std::stringstream error;
    
    error << "HTTP/1.0 " + itos(response.statusCode) + " whatever\r\n";
    error << "Content-Type: text/html\r\n";
    error << "Content-Length: " << html.str().size() << "\r\n";
    error << "Connection: close\r\n";
    error << "\r\n";
    error << html.str();
   
    std::string res = error.str();
    send(client_fd, res.c_str(), res.size() , MSG_NOSIGNAL);
    response.full = true;     

    return;
}
