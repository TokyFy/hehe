/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sranaivo <sranaivo@student.42antananarivo. +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 20:43:31 by sranaivo          #+#    #+#             */
/*   Updated: 2025/12/27 00:00:00 by sranaivo         ###   ########.fr       */
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
#include <signal.h>
#include <ctime>

static const int CGI_TIMEOUT_SEC = 30;

// Global map: pipe fd -> client fd
std::map<int, int> g_cgiPipeToClient;

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

char** setupEnv(Client &client, HttpServer &server, std::string &script_path)
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
    
    return vectorToCharArray(env);
}

bool startCgi(Client &client, HttpServer &server, int epoll_fd)
{
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
        return false;
    }
    
    // Check if script exists
    if (access(script_path.c_str(), R_OK) != 0)
    {
        client.response.statusCode = 404;
        return false;
    }
    
    char **env = setupEnv(client, server, script_path);
    
    char *argv[] = {
        const_cast<char *>(location.getCgiPath().c_str()), 
        const_cast<char *>(script_path.c_str()), 
        NULL
    };
    
    int pipeIn[2] = {-1, -1};
    int pipeOut[2] = {-1, -1};
    
    // Use pipe2 with O_NONBLOCK to avoid fcntl
    if (pipe2(pipeIn, O_NONBLOCK) == -1)
    {
        freeCharArray(env);
        client.response.statusCode = 500;
        return false;
    }
    
    if (pipe2(pipeOut, O_NONBLOCK) == -1)
    {
        close(pipeIn[0]);
        close(pipeIn[1]);
        freeCharArray(env);
        client.response.statusCode = 500;
        return false;
    }
    
    pid_t pid = fork();
    if (pid == -1)
    {
        close(pipeIn[0]);
        close(pipeIn[1]);
        close(pipeOut[0]);
        close(pipeOut[1]);
        freeCharArray(env);
        client.response.statusCode = 500;
        return false;
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
    
    // Parent process
    freeCharArray(env);
    close(pipeIn[0]);
    close(pipeOut[1]);
    
    // Pipes are already non-blocking (created with pipe2 O_NONBLOCK)
    
    // Setup CGI state
    client.cgi.active = true;
    client.cgi.pid = pid;
    client.cgi.pipeIn = pipeIn[1];
    client.cgi.pipeOut = pipeOut[0];
    client.cgi.startTime = std::time(NULL);
    client.cgi.outputBuffer.clear();
    client.cgi.inputOffset = 0;
    
    // Store POST body if any
    if (client.request.methodName == "POST" && !client.request.body.empty())
        client.cgi.inputBuffer = client.request.body;
    else
        client.cgi.inputBuffer.clear();
    
    // Register pipes with epoll
    struct epoll_event ev;
    
    // Register output pipe for reading
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    ev.data.fd = pipeOut[0];
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipeOut[0], &ev) == -1)
    {
        close(pipeIn[1]);
        close(pipeOut[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        client.cgi.reset();
        client.response.statusCode = 500;
        return false;
    }
    g_cgiPipeToClient[pipeOut[0]] = client.client_fd;
    
    // Register input pipe for writing if we have data to send
    if (!client.cgi.inputBuffer.empty())
    {
        ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
        ev.data.fd = pipeIn[1];
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipeIn[1], &ev) == -1)
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pipeOut[0], NULL);
            g_cgiPipeToClient.erase(pipeOut[0]);
            close(pipeIn[1]);
            close(pipeOut[0]);
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            client.cgi.reset();
            client.response.statusCode = 500;
            return false;
        }
        g_cgiPipeToClient[pipeIn[1]] = client.client_fd;
    }
    else
    {
        // No input to send, close write pipe immediately
        close(pipeIn[1]);
        client.cgi.pipeIn = -1;
    }
    
    return true;
}

void handleCgiWrite(Client &client, int epoll_fd)
{
    if (!client.cgi.active || client.cgi.pipeIn == -1)
        return;
    
    size_t remaining = client.cgi.inputBuffer.size() - client.cgi.inputOffset;
    if (remaining == 0)
    {
        // Done writing, close pipe
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.cgi.pipeIn, NULL);
        g_cgiPipeToClient.erase(client.cgi.pipeIn);
        close(client.cgi.pipeIn);
        client.cgi.pipeIn = -1;
        return;
    }
    
    const char *data = client.cgi.inputBuffer.c_str() + client.cgi.inputOffset;
    ssize_t written = write(client.cgi.pipeIn, data, remaining);
    
    if (written > 0)
    {
        client.cgi.inputOffset += written;
        
        // Check if done
        if (client.cgi.inputOffset >= client.cgi.inputBuffer.size())
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.cgi.pipeIn, NULL);
            g_cgiPipeToClient.erase(client.cgi.pipeIn);
            close(client.cgi.pipeIn);
            client.cgi.pipeIn = -1;
        }
    }
    else if (written < 0)
    {
        // Write error, close pipe
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.cgi.pipeIn, NULL);
        g_cgiPipeToClient.erase(client.cgi.pipeIn);
        close(client.cgi.pipeIn);
        client.cgi.pipeIn = -1;
    }
}

void handleCgiRead(Client &client, int epoll_fd)
{
    if (!client.cgi.active || client.cgi.pipeOut == -1)
        return;
    
    char buffer[8192];
    ssize_t bytes_read = read(client.cgi.pipeOut, buffer, sizeof(buffer));
    
    if (bytes_read > 0)
    {
        client.cgi.outputBuffer.append(buffer, bytes_read);
    }
    else if (bytes_read == 0)
    {
        // EOF - CGI closed its stdout
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.cgi.pipeOut, NULL);
        g_cgiPipeToClient.erase(client.cgi.pipeOut);
        close(client.cgi.pipeOut);
        client.cgi.pipeOut = -1;
    }
    // bytes_read < 0: EAGAIN/EWOULDBLOCK, just wait for next event
}

void parseCgiOutput(Client &client)
{
    std::string &output = client.cgi.outputBuffer;
    
    // Parse CGI output
    size_t header_end = output.find("\r\n\r\n");
    if (header_end == std::string::npos)
        header_end = output.find("\n\n");
    
    if (header_end != std::string::npos)
    {
        std::string cgi_headers = output.substr(0, header_end);
        size_t body_start = (output.find("\r\n\r\n") != std::string::npos) 
                            ? header_end + 4 : header_end + 2;
        client.response.body = output.substr(body_start);
        
        // Check for Content-Type in CGI headers
        if (cgi_headers.find("Content-Type:") == std::string::npos &&
            cgi_headers.find("Content-type:") == std::string::npos)
        {
            client.response.mimeType = "html";
        }
        else
        {
            client.response.mimeType = "html";
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
        client.response.body = output;
        client.response.mimeType = "html";
        client.response.statusCode = 200;
    }
    
    client.response.contentLength = client.response.body.size();
}

void cleanupCgi(Client &client, int epoll_fd)
{
    if (!client.cgi.active)
        return;
    
    // Remove pipes from epoll and close
    if (client.cgi.pipeIn != -1)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.cgi.pipeIn, NULL);
        g_cgiPipeToClient.erase(client.cgi.pipeIn);
        close(client.cgi.pipeIn);
    }
    
    if (client.cgi.pipeOut != -1)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.cgi.pipeOut, NULL);
        g_cgiPipeToClient.erase(client.cgi.pipeOut);
        close(client.cgi.pipeOut);
    }
    
    // Kill and wait for child if still running
    if (client.cgi.pid > 0)
    {
        int status;
        int result = waitpid(client.cgi.pid, &status, WNOHANG);
        if (result == 0)
        {
            // Still running, kill it
            kill(client.cgi.pid, SIGKILL);
            waitpid(client.cgi.pid, &status, 0);
        }
    }
    
    client.cgi.reset();
}

void checkCgiStatus(std::map<int, Client> &clients, int epoll_fd)
{
    time_t now = std::time(NULL);
    
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        Client &client = it->second;
        
        if (!client.cgi.active)
            continue;
        
        // Check timeout
        if (now - client.cgi.startTime >= CGI_TIMEOUT_SEC)
        {
            cleanupCgi(client, epoll_fd);
            client.response.statusCode = 504; // Gateway Timeout
            client.cgi.done = true;
            
            // Re-add client to epoll in write mode to send error response
            struct epoll_event ev;
            ev.events = EPOLLOUT;
            ev.data.fd = client.client_fd;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client.client_fd, &ev);
            continue;
        }
        
        // Check if child has finished
        if (client.cgi.pid > 0)
        {
            int status;
            int result = waitpid(client.cgi.pid, &status, WNOHANG);
            
            if (result > 0)
            {
                // Child finished
                client.cgi.pid = -1;
                
                // Close pipes - data should already have been read via epoll events
                if (client.cgi.pipeOut != -1)
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.cgi.pipeOut, NULL);
                    g_cgiPipeToClient.erase(client.cgi.pipeOut);
                    close(client.cgi.pipeOut);
                    client.cgi.pipeOut = -1;
                }
                
                // Close input pipe if still open
                if (client.cgi.pipeIn != -1)
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.cgi.pipeIn, NULL);
                    g_cgiPipeToClient.erase(client.cgi.pipeIn);
                    close(client.cgi.pipeIn);
                    client.cgi.pipeIn = -1;
                }
                
                // Check exit status
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    parseCgiOutput(client);
                }
                else
                {
                    client.response.statusCode = 500;
                }
                
                client.cgi.active = false;
                client.cgi.done = true;
                
                // Re-add client to epoll in write mode to send response
                struct epoll_event ev;
                ev.events = EPOLLOUT;
                ev.data.fd = client.client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client.client_fd, &ev);
            }
        }
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
