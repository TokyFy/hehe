/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 14:47:22 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/16 15:26:19 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <iostream>
# include <ctime>
# include <cstdlib>
# include <utility>
# include <sys/epoll.h>
# include <sys/socket.h>
# include "HttpRequest.hpp"
# include "HttpResponse.hpp"
# include "HttpServer.hpp"

class HttpRequest;

class Client
{
    public:
        int             server_fd;
        HttpServer *    server_ptr;
        int             client_fd;
        bool            sent;
        std::string     entry;
        unsigned long   time;
        HttpRequest     request;
        HttpResponse    response;

        Client(void);
        Client(Client const &to_copy);
        Client &operator=(Client const &to_what);
        Client(int fd, int server);
        ~Client(void);
        void    set(void);
        int     checkHead(void);
        void    assignFds(int fd, int server);
        void    reset(void);
        int     checkPath(void);
        void    setServerPtr(HttpServer* ptr);
};

#endif
