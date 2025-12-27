/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 15:18:14 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/16 11:44:46 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "utils.hpp"
#include <cstring>

HttpResponse::HttpResponse(void)
{
    lastRead = 0;
    full = false;
    header = false;
    multipartEnd = false;
    lastUploaded = 0;
    contentLength = 0;
    statusCode = 0;
}

HttpResponse::~HttpResponse(void)
{
    
}

std::string HttpResponse::createHeader(void)
{
    return ("HTTP/1.0 " + intToString(statusCode) + " " + getStatusMessage() + "\r\n");
}

std::string HttpResponse::getStatusMessage(void)
{
    switch (statusCode)
    {
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 204:
            return "No Content";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Moved Temporarily";
        case 304:
            return "Not Modified";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 409:
            return "Conflict";
        case 411:
            return "Length Required";
        case 413:
            return "Request Entity Too Large";
        case 414:
            return "URI Too Long";
        case 431:
            return "Request Header Fields Too Large";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        default:
            return "Unknown Error";
    }        
}

std::string HttpResponse::getRightMimeType(std::string path)
{
    std::string type;
    size_t      index;
    std::string defaultType = "text";

    mimeType = defaultType;
    index = path.find_last_of('.');
    if (index == std::string::npos)
        return (defaultType);
    if (path[index + 1])
    {
        type = path.substr(index + 1);
        return (type);
    }
    else
        return (defaultType);
}

std::string HttpResponse::getContentType(void)
{
    if (mimeType == "html" || mimeType == "css" || mimeType == "javascript" || mimeType == "csv")
        mimeType = "text/" + mimeType;
    else if (mimeType == "json" || mimeType == "xml" || mimeType == "pdf" || mimeType == "octet-stream" || mimeType == "zip")
        mimeType = "application/" + mimeType;
    else if (mimeType == "jpeg" || mimeType == "png" || mimeType == "gif")
        mimeType = "image/" + mimeType;
    else if (mimeType == "mpeg" || mimeType == "wav")
        mimeType = "audio/" + mimeType; 
    else if (mimeType == "mp4")
        mimeType = "video/" + mimeType;
    else
        return ("text/plain");
    return (mimeType);
}

std::string HttpResponse::getPairs(HttpRequest request)
{
    if (request.methodName == "DELETE" && statusCode == 204)
    {
        full = true;
        return ("");
    }
    std::string length;
    contentType = "Content-type: " + getContentType() + "\r\n";
    length = "Content-Length: " + intToString(contentLength) + "\r\n";
    return (connection + contentType + length);
}

std::string HttpResponse::getBody(void)
{
    std::string     line;
    char            buffer[8192];
    
    if (static_cast<unsigned long>(lastRead) <= contentLength)
    {
        std::memset(buffer, 0, 8192);
        file.read(buffer, 8192);
        line.append(buffer, 8192);
        lastRead = file.tellg();
        if (lastRead == -1)
        {
            full = true;
            file.close();            
        }
    }
    else
    {
        full = true;
        file.close();
    }
    return (line);
}

std::string HttpResponse::createResponse(HttpRequest request)
{
    std::string ret = createHeader() + getPairs(request) + "\r\n";
    header = true;
    return (ret);
}

void    HttpResponse::deleteMethod(std::string path)
{
    int             status;

    statusCode = 204;
    status = remove(path.c_str());
    if (status != 0)
    {
        statusCode = 409;
    }
}

std::string HttpResponse::rightMethod(void)
{
    if (method == "GET")
        return (getBody());
    return ("");
}

int HttpResponse::getContentLenght(void)
{
    return (contentLength);
}

void    HttpResponse::openFile(std::string path)
{
    if (mimeType != "html")
    {
        if (method == "POST")
        {
            uploadFile.open(path.c_str(), std::ios::binary);
            return ;
        }
        else
            file.open(path.c_str(), std::ios::binary);
    }
    else
    {
        if (method == "POST")
        {
            uploadFile.open(path.c_str());
            return;
        }
        else
            file.open(path.c_str());
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    contentLength = size;
}

HttpResponse &HttpResponse::operator=(HttpResponse const &to_what)
{
    if (this != &to_what)
    {
        method = to_what.method;
        protocol = to_what.protocol;
        statusCode = to_what.statusCode;
        body = to_what.body;
        responseHeader = to_what.responseHeader;
        contentType = to_what.contentType;
        connection = to_what.connection;
        contentLength = to_what.contentLength;
        response = to_what.response;
        mimeType = to_what.mimeType;
        full = to_what.full;
        header = to_what.header;
        file.close();
        lastRead = to_what.lastRead;
    }
    return (*this);
}

void    HttpResponse::uploadBody(void)
{
    uploadFile << body;
}
