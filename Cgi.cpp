/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sranaivo <sranaivo@student.42antananarivo. +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 20:43:31 by sranaivo          #+#    #+#             */
/*   Updated: 2025/12/19 08:39:30 by sranaivo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"
#include "Client.hpp"
#include "HttpServer.hpp"
#include "utils.hpp"
#include <cstring>

void    pipeEpoll(struct epoll_event &events, int &epoll_fd, Client &client, int pipeIn[2], int pipeOut[2])
{
    if (client.request.methodName == "POST")
    {
        events.events = EPOLLOUT;
        // events.data.ptr =
    }
}

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

char**  setupEnv(Client &client, HttpServer &server)
{
    std::vector<std::string> env;

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.0");
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("REQUEST_METHOD=" + client.request.methodName);
    env.push_back("QUERY_STRING=");
    if (!client.request.query.empty())
        env.push_back("QUERY_STRING=" + client.request.query);
    env.push_back("SERVER_NAME=localhost");
    env.push_back("SERVER_PORT=" + intToString(server.getPort()));
    //SCRIPT NAME MILA JERENA
    env.push_back("SCRIPT_NAME=/cgi-bin/script.py");
    // env.push_back("SCRIPT_NAME=" + script_path);
    env.push_back("REMOTE_ADDR=" + server.getInterface());
    if (client.request.methodName == "POST")
    {
        env.push_back("CONTENT_TYPE=" + client.request.getType());
        env.push_back("CONTENT_LENGTH=" + client.response.contentLength);
    }
    return (vectorToCharArray(env));
}


void    handleCgi(Client &client, HttpServer &server, int &epoll_fd, struct epoll_event &events)
{
    char **env;
    char **argv;
    int pipeIn[2];
    int pipeOut[2];
    pid_t pid;

    env = setupEnv(client, server);
    Location &location = server.getLocation(client.request.path);
    // chemin complet du script script_path (location.getCgiExt????)
    std::string script_path = location.getCgiExt() + client.request.path;
    char *argv[] = {const_cast<char *>(location.getCgiPath().c_str()), const_cast<char *>(script_path.c_str()), NULL};
    if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1)
        return;
    pid = fork();
    if (pid == 0)
    {
        dup2(pipeIn[0], STDIN_FILENO);
        dup2(pipeOut[1], STDOUT_FILENO);
        close(pipeIn[1]);
        close(pipeOut[0]);
        execve(location.getCgiPath().c_str(), argv, env);
        close(pipeIn[0]);
        close(pipeOut[1]);
    }
    else
    {
        close(pipeIn[0]);
        close(pipeOut[1]);
    }
}

// // Créer des pipes pour stdin/stdout
// int pipe_in[2];   // pour envoyer le body au CGI
// int pipe_out[2];  // pour recevoir la réponse du CGI

// pipe(pipe_in);
// pipe(pipe_out);

// pid_t pid = fork();

// if (pid == 0) {  // Processus enfant
//     // Rediriger stdin/stdout
//     dup2(pipe_in[0], STDIN_FILENO);
//     dup2(pipe_out[1], STDOUT_FILENO);
    
//     // Fermer les descripteurs inutilisés
//     close(pipe_in[0]);
//     close(pipe_in[1]);
//     close(pipe_out[0]);
//     close(pipe_out[1]);
    
//     // Préparer argv et envp
//     char *argv[] = {"/usr/bin/python3", "script.py", NULL};
//     char *envp[] = {"REQUEST_METHOD=GET", "QUERY_STRING=name=test", ..., NULL};
    
//     // Exécuter le script
//     execve("/usr/bin/python3", argv, envp);
//     exit(1);  // Si execve échoue
// }

// // Processus parent
// close(pipe_in[0]);
// close(pipe_out[1]);

// // Écrire le body de la requête dans pipe_in[1] si POST
// // Lire la réponse depuis pipe_out[0]
// // Utiliser waitpid() pour attendre la fin du CGI
// ```

// ### 4. **Gérer les timeouts**
// Un script CGI peut être lent ou planter. Vous devez :
// - Implémenter un timeout (5-30 secondes typiquement)
// - Utiliser `waitpid()` avec `WNOHANG` pour vérifier si le processus est terminé
// - Tuer le processus (`kill()`) s'il dépasse le timeout

// ### 5. **Parser la réponse du CGI**
// Le script CGI renvoie :
// ```
// Content-Type: text/html

// <!DOCTYPE html>
// <html>...