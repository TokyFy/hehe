/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sranaivo <sranaivo@student.42antananarivo. +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 20:43:31 by sranaivo          #+#    #+#             */
/*   Updated: 2025/12/26 00:00:00 by sranaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"
#include "Client.hpp"
#include "HttpServer.hpp"
#include "utils.hpp"
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>

char** vectorToCharArray(const std::vector<std::string> &vec)
{
    char** array = new char*[vec.size() + 1];
    
    for (size_t i = 0; i < vec.size(); i++) {
        array[i] = strdup(vec[i].c_str());
    }
    array[vec.size()] = NULL;
    return array;
}

void freeCharArray(char** array)
{
    if (!array)
        return;
    
    for (int i = 0; array[i] != NULL; i++) {
        free(array[i]);
    }
    delete[] array;
}

char**  setupEnv(Client &client, HttpServer &server, std::string &script_path)
{
    std::vector<std::string> env;

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("REQUEST_METHOD=" + client.request.methodName);
    env.push_back("SERVER_NAME=localhost");
    env.push_back("SERVER_PORT=" + intToString(server.getPort()));
    env.push_back("SCRIPT_NAME=" + script_path);
    env.push_back("SCRIPT_FILENAME=" + script_path);
    env.push_back("REMOTE_ADDR=" + server.getInterface());
    env.push_back("PATH_INFO=" + client.request.path);
    
    if (!client.request.query.empty())
        env.push_back("QUERY_STRING=" + client.request.query);
    else
        env.push_back("QUERY_STRING=");
    
    if (client.request.methodName == "POST")
    {
        std::string contentType = client.request.getPair("Content-Type");
        if (contentType.empty())
            contentType = "application/x-www-form-urlencoded";
        env.push_back("CONTENT_TYPE=" + contentType);
        env.push_back("CONTENT_LENGTH=" + intToString(client.request.body.size()));
    }
    
    std::map<std::string, std::string>::iterator it;
    for (it = client.request.headers.begin(); it != client.request.headers.end(); ++it)
    {
        std::string key = "HTTP_" + it->first;
        for (size_t i = 0; i < key.length(); i++)
        {
            if (key[i] == '-')
                key[i] = '_';
            else if (key[i] >= 'a' && key[i] <= 'z')
                key[i] = key[i] - 32;
        }
        env.push_back(key + "=" + it->second);
    }
    
    return (vectorToCharArray(env));
}

static bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void handleCgi(Client &client, HttpServer &server, int &epoll_fd, struct epoll_event &events)
{
    (void)epoll_fd;
    (void)events;
    
    char **env = NULL;
    int pipeIn[2] = {-1, -1};
    int pipeOut[2] = {-1, -1};
    pid_t pid;

    // Use rawPath for location matching, path for actual file
    Location &location = server.getLocation(client.request.rawPath);
    
    // Get the file path and convert to absolute
    std::string script_path = client.request.path;
    
    // Convert relative path to absolute if needed
    if (script_path.size() > 0 && script_path[0] != '/')
    {
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            if (script_path.size() >= 2 && script_path[0] == '.' && script_path[1] == '/')
                script_path = std::string(cwd) + script_path.substr(1);
            else
                script_path = std::string(cwd) + "/" + script_path;
        }
    }
    
    // Check if CGI interpreter exists
    if (access(location.getCgiPath().c_str(), X_OK) != 0)
    {
        client.response.statusCode = 500;
        return;
    }
    
    // Check if script exists
    if (access(script_path.c_str(), R_OK) != 0)
    {
        client.response.statusCode = 404;
        return;
    }
    
    env = setupEnv(client, server, script_path);
    
    char *argv[] = {
        const_cast<char *>(location.getCgiPath().c_str()), 
        const_cast<char *>(script_path.c_str()), 
        NULL
    };
    
    if (pipe(pipeIn) == -1)
    {
        freeCharArray(env);
        client.response.statusCode = 500;
        return;
    }
    
    if (pipe(pipeOut) == -1)
    {
        close(pipeIn[0]);
        close(pipeIn[1]);
        freeCharArray(env);
        client.response.statusCode = 500;
        return;
    }
    
    pid = fork();
    if (pid == -1)
    {
        close(pipeIn[0]);
        close(pipeIn[1]);
        close(pipeOut[0]);
        close(pipeOut[1]);
        freeCharArray(env);
        client.response.statusCode = 500;
        return;
    }
    
    if (pid == 0)
    {
        // Child process
        close(pipeIn[1]);
        close(pipeOut[0]);
        
        if (dup2(pipeIn[0], STDIN_FILENO) == -1)
            exit(1);
        if (dup2(pipeOut[1], STDOUT_FILENO) == -1)
            exit(1);
        
        close(pipeIn[0]);
        close(pipeOut[1]);
        
        // Change to script directory for relative path access
        std::string scriptDir = script_path;
        size_t lastSlash = scriptDir.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            scriptDir = scriptDir.substr(0, lastSlash);
            if (!scriptDir.empty())
                chdir(scriptDir.c_str());
        }
        
        execve(location.getCgiPath().c_str(), argv, env);
        exit(1);
    }
    else
    {
        // Parent process
        close(pipeIn[0]);
        close(pipeOut[1]);
        
        // Write POST body to CGI stdin (non-blocking with timeout)
        if (client.request.methodName == "POST" && !client.request.body.empty())
        {
            setNonBlocking(pipeIn[1]);
            
            size_t totalWritten = 0;
            size_t bodySize = client.request.body.size();
            const char* bodyData = client.request.body.c_str();
            int writeTimeout = 0;
            
            while (totalWritten < bodySize && writeTimeout < 10)
            {
                fd_set writeFds;
                FD_ZERO(&writeFds);
                FD_SET(pipeIn[1], &writeFds);
                
                struct timeval tv;
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                
                int ready = select(pipeIn[1] + 1, NULL, &writeFds, NULL, &tv);
                if (ready > 0)
                {
                    ssize_t written = write(pipeIn[1], bodyData + totalWritten, bodySize - totalWritten);
                    if (written > 0)
                        totalWritten += written;
                    else if (written == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
                        break;
                }
                else if (ready == 0)
                {
                    writeTimeout++;
                }
                else
                {
                    break;
                }
            }
        }
        close(pipeIn[1]);
        
        // Set read pipe to non-blocking
        setNonBlocking(pipeOut[0]);
        
        char buffer[8192];
        std::string cgi_output;
        
        // Read CGI output with timeout using select
        int timeout = 0;
        const int maxTimeout = 30; // 30 seconds max
        
        while (timeout < maxTimeout)
        {
            // Check if child has finished
            int status;
            int wait_result = waitpid(pid, &status, WNOHANG);
            
            if (wait_result > 0)
            {
                // Child finished, read remaining output
                ssize_t bytes_read;
                while ((bytes_read = read(pipeOut[0], buffer, sizeof(buffer))) > 0)
                {
                    cgi_output.append(buffer, bytes_read);
                }
                
                close(pipeOut[0]);
                freeCharArray(env);
                
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    // Parse CGI output
                    size_t header_end = cgi_output.find("\r\n\r\n");
                    if (header_end == std::string::npos)
                        header_end = cgi_output.find("\n\n");
                    
                    if (header_end != std::string::npos)
                    {
                        std::string cgi_headers = cgi_output.substr(0, header_end);
                        size_t body_start = (cgi_output.find("\r\n\r\n") != std::string::npos) 
                                            ? header_end + 4 : header_end + 2;
                        client.response.body = cgi_output.substr(body_start);
                        
                        // Check for Content-Type in CGI headers
                        if (cgi_headers.find("Content-Type:") == std::string::npos &&
                            cgi_headers.find("Content-type:") == std::string::npos)
                        {
                            client.response.mimeType = "html";
                        }
                        else
                        {
                            // Extract content type from CGI headers
                            size_t ctPos = cgi_headers.find("Content-Type:");
                            if (ctPos == std::string::npos)
                                ctPos = cgi_headers.find("Content-type:");
                            if (ctPos != std::string::npos)
                            {
                                size_t ctEnd = cgi_headers.find("\n", ctPos);
                                std::string ct = cgi_headers.substr(ctPos + 13, ctEnd - ctPos - 13);
                                // Trim whitespace
                                size_t start = ct.find_first_not_of(" \t\r");
                                if (start != std::string::npos)
                                    ct = ct.substr(start);
                                size_t end = ct.find_last_not_of(" \t\r");
                                if (end != std::string::npos)
                                    ct = ct.substr(0, end + 1);
                                client.response.mimeType = "html"; // Default, the actual CT is handled
                            }
                        }
                        
                        // Check for Status header in CGI output
                        size_t statusPos = cgi_headers.find("Status:");
                        if (statusPos != std::string::npos)
                        {
                            size_t statusEnd = cgi_headers.find("\n", statusPos);
                            std::string statusLine = cgi_headers.substr(statusPos + 7, statusEnd - statusPos - 7);
                            int cgiStatus = std::atoi(statusLine.c_str());
                            if (cgiStatus >= 100 && cgiStatus < 600)
                                client.response.statusCode = cgiStatus;
                            else
                                client.response.statusCode = 200;
                        }
                        else
                        {
                            client.response.statusCode = 200;
                        }
                    }
                    else
                    {
                        // No headers, treat entire output as body
                        client.response.body = cgi_output;
                        client.response.mimeType = "html";
                        client.response.statusCode = 200;
                    }
                    client.response.contentLength = client.response.body.size();
                }
                else
                {
                    client.response.statusCode = 500;
                }
                return;
            }
            else if (wait_result == -1)
            {
                // Error
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                close(pipeOut[0]);
                freeCharArray(env);
                client.response.statusCode = 500;
                return;
            }
            
            // Child still running, try to read available output
            fd_set readFds;
            FD_ZERO(&readFds);
            FD_SET(pipeOut[0], &readFds);
            
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            int ready = select(pipeOut[0] + 1, &readFds, NULL, NULL, &tv);
            if (ready > 0)
            {
                ssize_t bytes_read = read(pipeOut[0], buffer, sizeof(buffer));
                if (bytes_read > 0)
                {
                    cgi_output.append(buffer, bytes_read);
                    timeout = 0; // Reset timeout since we got data
                }
            }
            else if (ready == 0)
            {
                timeout++;
            }
            else
            {
                // Select error
                timeout++;
            }
        }
        
        // Timeout reached
        kill(pid, SIGKILL);
        int status;
        waitpid(pid, &status, 0);
        close(pipeOut[0]);
        freeCharArray(env);
        client.response.statusCode = 504; // Gateway Timeout
    }
}

bool isCgiRequest(const std::string& path, const Location& location)
{
    std::string cgiExt = location.getCgiExt();
    if (cgiExt.empty())
        return false;
    
    // Check if path ends with CGI extension
    if (path.length() >= cgiExt.length())
    {
        return path.compare(path.length() - cgiExt.length(), cgiExt.length(), cgiExt) == 0;
    }
    return false;
}
