/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 15:16:08 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/18 09:54:37 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include <cstdio>
# include <fstream>
# include <string>
# include <unistd.h>

class HttpRequest;

class HttpResponse
{
    public:
        std::string method;
        std::string protocol;
        int         statusCode;
        std::string body;
        std::string responseHeader;
        std::string contentType;
        std::string connection;
        size_t      contentLength;
        std::string response;
        std::string mimeType;
        bool        full;
        bool            header;
        bool            multipartEnd;
        std::ifstream   file;
        std::ofstream   uploadFile;
        ssize_t         lastRead;
        ssize_t         lastUploaded;
        std::ofstream   multipartFile;

        HttpResponse(void);
        ~HttpResponse(void);
        HttpResponse &operator=(HttpResponse const &to_what);
        std::string createHeader(void);
        std::string getStatusMessage(void);
        std::string postBody(void);
        std::string getBody(void);
        std::string createResponse(HttpRequest request);
        std::string rightMethod(void);
        void        deleteMethod(std::string path);
        std::string getPairs(HttpRequest request);
        std::string getRightMimeType(std::string path);
        int         getContentLenght(void);
        std::string getContentType(void);
        void        openFile(std::string path);
        void        uploadBody(void);
};

#endif
