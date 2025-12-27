/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sranaivo <sranaivo@student.42antananarivo. +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 14:41:34 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/18 22:13:14 by sranaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

HttpRequest::HttpRequest(void)
{
    full = false;
    fullHeader = false;
    multipart = false;
    body = "";
    boundary = "";
    query = "";
}

HttpRequest::~HttpRequest(void)
{

}

HttpRequest::HttpRequest(HttpRequest const &to_copy)
{
    *this = to_copy;
}

HttpRequest &HttpRequest::operator=(HttpRequest const &to_what)
{
    if (this != &to_what)
    {
        full = to_what.full;
        fullHeader = to_what.fullHeader;
        multipart = to_what.multipart;
        methodName = to_what.methodName;
        path = to_what.path; 
        protocol = to_what.protocol;
        body = to_what.body;
        headers = to_what.headers;
        boundary = to_what.boundary;
        query = to_what.query;
    }
    return (*this);
}

int    HttpRequest::parseHead(std::string head, HttpResponse &response)
{
    if (head.empty() || head.find_first_not_of(" \t\r\n") == std::string::npos)
        return (400);
    if (head.size() > 8192)
        return (414);
    
    std::stringstream ss(head);
    char del = ' ';
    std::string headers[4];
    int i = 0;
    
    for (; i < 3; i++)
        getline(ss, headers[i], del);
    
    if (headers[0].empty() || headers[1].empty() || headers[2].empty())
        return (400);
    
    methodName = headers[0];
    if (!isValidMethod(methodName))
        return (405);
    
    path = urlDecode(headers[1]);
    if (!isValidPath(path))
        return (400);
    
    if (path.find('?') != std::string::npos)
        setQueryString();
    
    protocol = headers[2];
    if (!protocol.empty() && protocol[protocol.size() - 1] == '\r')
        protocol = protocol.substr(0, protocol.size() - 1);
    
    if (getline(ss, headers[3], del) && !headers[3].empty())
    {
        std::string trimmed = headers[3];
        size_t start = trimmed.find_first_not_of(" \t\r\n");
        if (start != std::string::npos)
            return (400);
    }
    
    if (!isUpper(methodName))
        return (400);
    if (!isValidProtocol(protocol))
        return (400);
    
    response.method = methodName;
    return (0);
}

void    HttpRequest::fillPair(std::string pair)
{
    if (pair.empty())
        return ;
    
    size_t colonPos = pair.find(':');
    if (colonPos == std::string::npos || colonPos == 0)
        return ;
    
    std::string name = pair.substr(0, colonPos);
    std::string value;
    
    if (colonPos + 1 < pair.size())
    {
        value = pair.substr(colonPos + 1);
        size_t start = value.find_first_not_of(" \t");
        size_t end = value.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos)
            value = value.substr(start, end - start + 1);
        else if (start != std::string::npos)
            value = value.substr(start);
        else
            value.clear();
    }
    
    if (!isValidHeaderName(name) || !isValidHeaderValue(value))
        return ;
    
    headers.insert(std::pair<std::string, std::string>(name, value));
}

void   HttpRequest::parseHeaders(std::string rest)
{
    std::string cut = "\r\n\r\n";
    size_t      end = rest.find(cut);
    rest = rest.substr(0, end);
    size_t pos = 0;
    std::string delimiter = "\r\n";
    while ((pos = rest.find(delimiter)) != std::string::npos)
    {
        fillPair(rest.substr(0, pos));
        rest.erase(0, pos + delimiter.length());
    }
    fillPair(rest);
}

int    HttpRequest::parse(std::string request)
{
    size_t pos = request.find("\r");
    if (pos == std::string::npos)
        return (0);
    parseHeaders(request.substr(pos + 2));
    return (0);
}

std::string HttpRequest::getPair(std::string entry)
{
    std::map<std::string, std::string>::iterator it;

    it = headers.find(entry);
    if (it != headers.end())
        return (it->second);
    return ("");
}

void    HttpRequest::checkMultipart(void)
{
    std::string content;
    std::string type;
    std::string delim = ";";
    std::string bound = "boundary=";
    content = getPair("Content-Type");
    size_t boundPos = content.find(bound);
    size_t pos = content.find(delim);
    if (pos != std::string::npos)
        type = content.substr(0, pos);
    if (type == "multipart/form-data")
    {
        multipart = true;
    }
    if (boundPos != std::string::npos)
        boundary = "--" + content.substr(boundPos + bound.size());
}

std::string HttpRequest::getType(void)
{
    contentType = getPair("Content-Type");
    return (contentType);
}

void    HttpRequest::setQueryString(void)
{
    size_t pos;
    pos = path.find('?');
    if (pos != std::string::npos)
    {
        if (pos + 1 < path.length())
            query = path.substr(pos + 1);
        path = path.substr(0, pos);
    }
}
