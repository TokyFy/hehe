/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpAgent.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: franaivo <franaivo@student.42antananarivo  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 07:57:20 by franaivo          #+#    #+#             */
/*   Updated: 2025/11/06 08:47:18 by franaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# pragma once

enum TYPE   { SERVER , CLIENT };

class HttpAgent {
    private:
        HttpAgent();

    protected :
        int socket_fd;
        TYPE type;

    public :
        HttpAgent( int socket_fd , TYPE type);
        int getSocketFd() const;
        TYPE getType() const;
        virtual ~HttpAgent();
};

