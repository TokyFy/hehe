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

# include <sys/epoll.h>
# include <sys/socket.h>
# include <sys/types.h>
# include "HttpRequest.hpp"
# include "HttpResponse.hpp"
# include "HttpServer.hpp"

class HttpRequest;

// CGI state for async handling
struct CgiState
{
    bool        active;
    bool        done;           // CGI completed, response ready
    pid_t       pid;
    int         pipeIn;         // Write to CGI stdin
    int         pipeOut;        // Read from CGI stdout
    std::string inputBuffer;    // Data to write to CGI
    size_t      inputOffset;    // How much we've written
    std::string outputBuffer;   // Data read from CGI
    time_t      startTime;      // For timeout

    CgiState() : active(false), done(false), pid(-1), pipeIn(-1), pipeOut(-1), inputOffset(0), startTime(0) {}
    
    void reset()
    {
        active = false;
        done = false;
        pid = -1;
        pipeIn = -1;
        pipeOut = -1;
        inputBuffer.clear();;
        inputOffset = 0;
        outputBuffer.clear();
        startTime = 0;
    }
};

// Upload state for streaming large files
struct UploadState
{
    enum Phase { WAITING_HEADERS, PARSING_PART_HEADERS, STREAMING_CONTENT, DONE };
    
    Phase           phase;
    std::string     boundary;
    std::string     buffer;         // Small buffer for boundary detection
    std::string     filename;
    std::string     uploadPath;
    std::ofstream   file;
    size_t          bytesReceived;
    size_t          contentLength;
    bool            active;
    
    UploadState() : phase(WAITING_HEADERS), bytesReceived(0), contentLength(0), active(false) {}
    
    void reset()
    {
        phase = WAITING_HEADERS;
        boundary.clear();
        buffer.clear();
        filename.clear();
        uploadPath.clear();
        if (file.is_open())
            file.close();
        bytesReceived = 0;
        contentLength = 0;
        active = false;
    }
};

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
        CgiState        cgi;
        UploadState     upload;

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
        void    redirect(const std::string& path);
        void    sendStatusPage();
        void    error();
};


#endif
