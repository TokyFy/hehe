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
# include <sys/epoll.h>
# include "Client.hpp"
# include "HttpServer.hpp"

class Client;
class HttpServer;
class Location;

void    handleCgi(Client &client, HttpServer &server, int &epoll_fd, struct epoll_event &events);
char**  setupEnv(Client &client, HttpServer &server, std::string &script_path);
void    freeCharArray(char** array);
char**  vectorToCharArray(const std::vector<std::string> &vec);
bool    isCgiRequest(const std::string& path, const Location& location);

#endif
