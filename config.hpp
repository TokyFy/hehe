/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 09:52:28 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/18 14:45:05 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cstring>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <utility>
#include <vector> 
#include <iostream>
#include <sstream>
#include <climits>
#include <cstdlib>
#include "HttpServer.hpp"

struct s_location_config
{
    std::string index;
    bool        autoindex;
    std::string root;
    int         allow_methods;
    std::string cgi_path;
    std::string cgi_ext;
    std::string error_pages[0x255];
};

struct s_server_config
{
    ssize_t     client_max_body_size;
    int         port;
    std::string interface;
    s_location_config*   locations[0x0F];
};

enum TokenType {
                                // Structural tokens
    TOKEN_LBRACE,               // {
    TOKEN_RBRACE,               // }
    TOKEN_SEMICOLON,            // ;
    TOKEN_RETURN,               // \n
    
                                // Comments
    TOKEN_COMMENT,              // # This is a comment

                                // Keywords (directives)
    TOKEN_LOCATION,             // location
    TOKEN_LISTEN,               // listen
    TOKEN_ROOT,                 // root
    TOKEN_INDEX,                // index
    TOKEN_CLIENT_MAX_BODY_SIZE, // client_max_body_size
    TOKEN_ERROR_PAGE,           // error_page
    TOKEN_ALLOW_METHODS,        // allow_methods
    TOKEN_AUTOINDEX,            // autoindex
    TOKEN_CGI,                  // cgi
    TOKEN_UPLOAD,               // upload


                                // Values
    TOKEN_WORDS,
    TOKEN_EOF
};



std::vector<HttpServer> parser(std::string path);

#endif
