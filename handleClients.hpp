/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleClients.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 10:01:22 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/18 10:02:41 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HANDLE_CLIENTS_HPP
# define HANDLE_CLIENTS_HPP

#include "Client.hpp"
#include "HttpServer.hpp"
#include "Cgi.hpp"
#include "utils.hpp"
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>

/**
 * Main server event loop
 * Uses single epoll() for all I/O operations
 */
void multiple(std::vector<HttpServer> &servers);

/**
 * Check and disconnect timed-out clients
 */
void checkTimeout(std::map<int, Client> &clients, int epoll_fd);

#endif