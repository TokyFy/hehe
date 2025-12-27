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
    return ("HTTP/1.0 " + intToString(statusCode) + getStatusMessage());
}

std::string HttpResponse::getStatusMessage(void)
{
    switch (statusCode)
    {
        case 200:
            return (" OK\r\n");
        case 201:
            return (" Created\r\n");
        case 202:
            return (" Accepted\r\n");
        case 204:
            return (" No content\r\n");
        case 301:
            return (" Moved Permanently\r\n");
        case 302:
            return (" Moved Temporarily\r\n");
        case 304:
            return (" Not Modified\r\n");
        case 400:
            return (" Bad Request\r\n");
        case 401:
            return (" Unauthorized\r\n");
        case 403:
            return (" Forbidden\r\n");
        case 404:
            return (" Not Found\r\n");
        case 405:
            return (" METHOD NOT ALLOWED\r\n");
        case 409:
            return (" CONFLICT\r\n");
        case 411:
            return (" Length Required\r\n");
        case 413:
            return (" Request Entity Too Large\r\n");
        case 500:
            return (" Internal Server Error\r\n");
        case 501:
            return (" Not Implemented\r\n");
        case 502:
            return (" Bad Gateway\r\n");
        case 503:
            return (" Service Unavailable\r\n");
        default:
            return (" HEHEHEHEHE\r\n");
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
