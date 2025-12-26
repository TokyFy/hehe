/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: franaivo <franaivo@student.42antananarivo  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 21:17:55 by franaivo          #+#    #+#             */
/*   Updated: 2025/11/06 09:04:27 by franaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# pragma once

#include <map>
#include <string>
#include <sys/types.h>
#include <vector>
#include "HttpAgent.hpp"


class Location
{
    private:
        std::string source;
        std::string index;
        bool        autoindex;
        std::string root;

        // Bit masking
        int         allow_methods;
        std::string cgi_path;
        std::string cgi_ext;
        std::string upload;
        std::string redirect_url;
        int         redirect_code;

    public:
        Location();
        ~Location();

        const std::string&  getSource() const;
        void                setSource(const std::string& value);

        const std::string&  getIndex() const;
        void                setIndex(const std::string& value);
        bool                getAutoIndex() const;
        void                setAutoIndex(bool value);
        const std::string&  getRoot() const;
        void                setRoot(const std::string& value);

        const std::string&  getCgiPath() const;
        void                setCgiPath(const std::string& value);
        const std::string&  getCgiExt() const;
        void                setCgiExt(const std::string& value);

        const std::string&  getUploadPath() const;
        void                setUploadPath(const std::string& path);

        const std::string&  getRedirectUrl() const;
        void                setRedirectUrl(const std::string& url);
        int                 getRedirectCode() const;
        void                setRedirectCode(int code);

        void                addAllowedMethod(const std::string&);
        bool                isAllowedMethod(const std::string&);
        void                normalize();
};

class HttpServer : public HttpAgent
{
    private :
        HttpServer();
        std::string name;
        ssize_t     client_max_body_size;
        int         port;
        std::string interface;
        std::vector<Location> locations;
        std::map<int , std::string> error_pages;
        

    public :
        ~HttpServer();
        HttpServer(int socket_fd);
        HttpServer(int socket_fd , std::string name);

        ssize_t             getClientMaxBodySize() const;
        void                setClientMaxBodySize(ssize_t value);
        int                 getPort() const;
        void                setPort(int value);
        const std::string&  getInterface() const;
        void                setInterface(const std::string& value);

                const std::string&  getName() const;
        void                setName(const std::string& value);

        void                setErrorPage(int code , std::string path);
        const std::string  getErrorPage(int code);
        void                addLocation(Location& location);
        Location&           getLocation(std::string& path);
        std::string         getRoutedPath(const std::string& requestedPath) const;

        void                setToEppoll(int epoll_fd, struct epoll_event &events);
        void                normalize();
};

