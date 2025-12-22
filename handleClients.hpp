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
#include <map>

void multiple(std::vector<HttpServer> &servers);


#endif