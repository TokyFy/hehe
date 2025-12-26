/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sranaivo <sranaivo@student.42antananarivo. +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 14:32:09 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/18 22:13:44 by sranaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include <iostream>
# include <sstream>
# include <string>
# include <map>
# include <vector>
# include "HttpResponse.hpp"
# include "utils.hpp"

enum { GET, POST, DELETE };

class HttpResponse;

class HttpRequest
{
    public:
        std::string     methodName;
        std::string     path; 
        std::string     rawPath;
        std::string     protocol;
        std::map<std::string, std::string> headers;
        std::string     body;
        bool            full;
        bool            fullHeader;
        bool            multipart;
        std::string     query;
        std::string     boundary;
        std::string     contentType;

        HttpRequest(void);
        HttpRequest(HttpRequest const &to_copy);
        HttpRequest &operator=(HttpRequest const &to_what);
        ~HttpRequest(void);
        int     parse(std::string request);
        void    parseHeaders(std::string headers);
        int     parseHead(std::string head, HttpResponse &response);
        void    fillPair(std::string pair);
        std::string getPair(std::string entry);
        void    checkMultipart(void);
        std::string getType(void);
        void        setQueryString(void);
};

#endif
