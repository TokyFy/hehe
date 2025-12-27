/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: franaivo <franaivo@student.42antananarivo  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 20:43:31 by franaivo          #+#    #+#             */
/*   Updated: 2025/12/26 00:00:00 by franaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
# define CGI_HPP

# include <string>
# include <vector>
# include <map>
# include <sys/epoll.h>
# include "Client.hpp"
# include "HttpServer.hpp"

class Client;
class HttpServer;
class Location;

// Start CGI process and register pipes with epoll (non-blocking)
bool    startCgi(Client &client, HttpServer &server, int epoll_fd);

// Handle CGI pipe write event (write to CGI stdin)
void    handleCgiWrite(Client &client, int epoll_fd);

// Handle CGI pipe read event (read from CGI stdout)
void    handleCgiRead(Client &client, int epoll_fd);

// Check CGI processes for completion or timeout
void    checkCgiStatus(std::map<int, Client> &clients, int epoll_fd);

// Clean up CGI resources
void    cleanupCgi(Client &client, int epoll_fd);

// Parse CGI output and set response
void    parseCgiOutput(Client &client);

// Helper functions
char**  setupEnv(Client &client, HttpServer &server, std::string &script_path);
void    freeCharArray(char** array);
char**  vectorToCharArray(const std::vector<std::string> &vec);
bool    isCgiRequest(const std::string& path, const Location& location);

// Map to find client by CGI pipe fd
extern std::map<int, int> g_cgiPipeToClient;

#endif
