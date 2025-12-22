/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpAgent.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: franaivo <franaivo@student.42antananarivo  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 08:32:16 by franaivo          #+#    #+#             */
/*   Updated: 2025/11/06 10:24:40 by franaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpAgent.hpp"
#include <unistd.h>

HttpAgent::HttpAgent( int socket_fd , TYPE type)
    : socket_fd(socket_fd) , type(type) 
{
    return;
}

HttpAgent::~HttpAgent()
{
    return;
}

int HttpAgent::getSocketFd() const
{
    return socket_fd;
}

TYPE HttpAgent::getType() const 
{
    return type;
}
