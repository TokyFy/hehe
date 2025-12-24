/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 11:06:51 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/12 13:58:21 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.hpp"
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include "utils.hpp"
#include "HttpServer.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>


bool    isUpper(std::string str)
{
    std::string::iterator it;

    for (it = str.begin(); it != str.end(); ++it)
    {
        if (!isupper(*it))
            return (false);
    }
    return (true);
}

bool    isValidProtocol(std::string str)
{
    if (str != "HTTP/1.1" && str != "HTTP/1.0")
        return (false);
    return (true);
}

bool    isValidMethod(std::string str)
{
    if (str != "GET" && str != "POST" && str != "DELETE")
        return (false);
    return (true);
}

std::string intToString(int number)
{
    std::string         ret;
    std::stringstream   ss;

    ss << number;
    ret = ss.str();
    return (ret);
}

FILE_TYPE mime(const std::string& str)
{
    const char *path = str.c_str();

    // Check existence
    if (access(path, F_OK) == -1)
        return ERR_NOTFOUND;

    // Check readability
    if (access(path, R_OK) == -1)
        return ERR_DENIED;

    struct stat info;
    if (stat(path, &info) == -1)
        return BINARY; // fallback

    // Directory
    if (S_ISDIR(info.st_mode))
        return FOLDER;

    // Get file extension
    size_t pos = str.find_last_of('.');
    if (pos == std::string::npos)
        return BINARY;

    std::string ext = str.substr(pos);

    // Map extensions to FILE_TYPE
    if (ext == ".html" || ext == ".htm") return HTML;
    if (ext == ".txt")                   return TEXT;
    if (ext == ".css")                   return CSS;
    if (ext == ".js")                    return JS;
    if (ext == ".png")                   return PNG;
    if (ext == ".jpg" || ext == ".jpeg") return JPEG;
    if (ext == ".ico")                   return ICO;
    if (ext == ".mp4")                   return MP4;
    if (ext == ".mpeg" || ext == ".mpg") return MPEG;

    // Default
    return BINARY;
}

std::string replaceFirstOccurrence(const std::string &originalString, const std::string &target, const std::string &replacement)
{
    if (target.empty())
        return originalString;

    std::string resultString = originalString;
    std::string::size_type position = resultString.find(target);

    if (position != std::string::npos)
        resultString.replace(position, target.length(), replacement);

    return resultString;
}

std::string indexof(Location& location, std::string path)
{
    (void)(location);
    DIR *folder = opendir(path.c_str());

    std::cerr << "INDEX OF " << path << std::endl;

    if (!folder)
    {
        return
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 9\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Not Found";
    }

    struct dirent *entry;
    std::stringstream html;

    html << "<html><head>";
    html << "<title>Index of</title>";
    html << "<style> body { font-family: monospace; line-height: 1.2em; } </style>";
    html << "</head><body>";
    html << "<h3>Index of " << path << "</h3><hr>";

    while ((entry = readdir(folder)) != NULL)
    {
        if (std::strcmp(entry->d_name, ".") == 0 ||
            std::strcmp(entry->d_name, "..") == 0)
            continue;

        html << "<p><a href=\""
             << (path == "/" ? "" : path) << "/"
             << entry->d_name << "\">"
             << entry->d_name
             << "</a></p>";
    }

    html << "</body></html>";
    closedir(folder);

    std::string body = html.str();

    std::stringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}
