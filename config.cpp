/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/22 21:37:01 by franaivo          #+#    #+#             */
/*   Updated: 2025/12/18 14:46:10 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config.hpp"

unsigned int keyword_hash(const std::string& s)
{
    unsigned int h = 0;

    for (size_t i = 0; i < s.size(); i++)
        h = h * 31u + (unsigned char)s[i];
    return h;
}

const char* token_type_to_string(TokenType type)
{
    switch (type) {
        case TOKEN_ROOT:                  return "TOKEN_ROOT";
        case TOKEN_INDEX:                 return "TOKEN_INDEX";
        case TOKEN_AUTOINDEX:             return "TOKEN_AUTOINDEX";
        case TOKEN_LISTEN:                return "TOKEN_LISTEN";
        case TOKEN_LBRACE:                return "TOKEN_LBRACE";
        case TOKEN_RBRACE:                return "TOKEN_RBRACE";
        case TOKEN_RETURN:                return "TOKEN_RETURN";
        case TOKEN_COMMENT:               return "TOKEN_COMMENT";
        case TOKEN_CGI:                   return "TOKEN_CGI";
        case TOKEN_LOCATION:              return "TOKEN_LOCATION";
        case TOKEN_SEMICOLON:             return "TOKEN_SEMICOLON";
        case TOKEN_ERROR_PAGE:            return "TOKEN_ERROR_PAGE";
        case TOKEN_ALLOW_METHODS:         return "TOKEN_ALLOW_METHODS";
        case TOKEN_CLIENT_MAX_BODY_SIZE:  return "TOKEN_CLIENT_MAX_BODY_SIZE";
        case TOKEN_UPLOAD:                return "TOKEN_UPLOAD";
        case TOKEN_REDIRECT:              return "TOKEN_REDIRECT";
        case TOKEN_WORDS:                 return "TOKEN_WORDS";
        case TOKEN_EOF:                   return "TOKEN_EOF";
    }
    return "UNKNOWN_TOKEN";
}

TokenType get_token_type(std::string &token)
{
    switch (keyword_hash(token)) {
        case 0x003580E2u    : return TOKEN_ROOT;
        case 0x05FB28D2u    : return TOKEN_INDEX;
        case 0xBE490DC7u    : return TOKEN_LISTEN;
        case 0x0000007Bu    : return TOKEN_LBRACE;
        case 0x0000007Du    : return TOKEN_RBRACE;
        case 0x0000000Au    : return TOKEN_RETURN;
        case 0x00000023u    : return TOKEN_COMMENT;
        case 0x00018085u    : return TOKEN_CGI;
        case 0x714F9FB5u    : return TOKEN_LOCATION;
        case 0x0000003Bu    : return TOKEN_SEMICOLON;
        case 0x61844E66u    : return TOKEN_ERROR_PAGE;
        case 0x638F8083u    : return TOKEN_AUTOINDEX;
        case 0x0C3E3CBCu    : return TOKEN_ALLOW_METHODS;
        case 0x74F60DEFu    : return TOKEN_CLIENT_MAX_BODY_SIZE;
        case 0xCE040E01u    : return TOKEN_UPLOAD;
        case 0xC84E3D30u    : return TOKEN_REDIRECT;
        
        default             : return TOKEN_WORDS;
    }
}

std::vector<std::pair<TokenType, std::string> > tokenizer(std::ifstream &config)
{
    std::vector<std::pair<TokenType, std::string> > tokens;

    if(!config.is_open())
        return tokens;


    std::string line;

    while (std::getline(config , line))
    {
        const char *c = line.c_str();
        std::string token; 
        
        while (std::isspace(*c)) {
                    c++;
        }

        while (*c) {
            if(*c == ';' || *c == '\n' || *c == '#' || *c == '{' || *c == '}')
            {
                if(token.size() != 0)
                {
                    tokens.push_back(std::make_pair( get_token_type(token) , token)); 
                    token.clear();
                }
                token += *c;
                tokens.push_back(std::make_pair( get_token_type(token) , token)); 
                token.clear();

                c++;
                continue;
            }
            if(std::isspace(*c) && token.length() != 0)
            {
                tokens.push_back(std::make_pair( get_token_type(token) , token)); 
                token.clear();
                while (std::isspace(*c)) {
                    c++;
                }
                continue;
            }

            if(!std::isspace(*c))
                token += *c;
            c++;
        }

        if(token.length())
        {
            tokens.push_back(std::make_pair( get_token_type(token) , token)); 
            token.clear();
        }

        tokens.push_back(std::make_pair(TOKEN_RETURN , token));
    }

    tokens.push_back(std::make_pair(TOKEN_EOF, "EOF"));
    return tokens;
}

enum PARSER_STATE { NONE , BLOCK_SERVER , BLOCK_SERVER_ENTER , BLOCK_LOCATION , BLOCK_LOCATION_ENTER};

typedef std::pair<TokenType, std::string> t_token;

int safe_atoi(const std::string &s) {
    char *end;
    long value = std::strtol(s.c_str(), &end, 10);

    // If end doesn't point to the end of the string → invalid characters
    if (*end != '\0')
        throw std::runtime_error("Invalid number");

    // Check overflow for int
    if (value < INT_MIN || value > INT_MAX)
        throw std::runtime_error("Number out of range");

    if(value < 0)
        throw std::runtime_error("Nagative number");

    return static_cast<int>(value);
}


std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delimiter))
        result.push_back(item);

    return result;
}

// Parsing
void parse_listen(HttpServer *server , std::vector<t_token> &tokens)
{
    if(tokens[0].first != TOKEN_LISTEN)
        throw std::runtime_error(server->getName() + " : listen format : listen INTERFACE:PORT");

    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error(server->getName() + " : Listen should have a value");

    std::vector<std::string> splited = split(tokens[0].second , ':');

    if(splited.size() != 2)
        throw std::runtime_error(server->getName() + " : Listed syntax : eg : 0.0.0.0:8080");

    server->setInterface(splited[0]);
    
    try 
    {
        server->setPort(safe_atoi(splited[1]));
    }
    catch(std::exception &e)
    {
        throw std::runtime_error(server->getName() + " : Port : " + std::string(e.what()));
    }

    tokens.erase(tokens.begin());
    
    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(server->getName() + " : Missing ; in the end of Listed");

    tokens.erase(tokens.begin());

    std::cerr << "Interface : " << server->getInterface() << std::endl;
    std::cerr << "Port : " << server->getPort() << std::endl;
}

long parse_client_max_body_size(const std::string &s) {
    if (s.empty())
        throw std::runtime_error("Empty size string");

    char *end;
    long value = std::strtol(s.c_str(), &end, 10);

    if (end == s.c_str())
        throw std::runtime_error("Invalid size: no digits");

    long multiplier = 1;

    if (*end != '\0') {
        if (*end == 'M') {
            multiplier = 1024L * 1024L; // 1 MB
        } else {
            throw std::runtime_error("Invalid suffix: only 'M' allowed");
        }
        if (*(end + 1) != '\0')
            throw std::runtime_error("Invalid characters after suffix");
    }
    if (value < 0 || value > LONG_MAX / multiplier)
        throw std::runtime_error("Size too large");

    return value * multiplier;
}

void parse_client_max_body_size(HttpServer *server , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin());

    try 
    {
        ssize_t max_body = parse_client_max_body_size(tokens[0].second);

        if(max_body < 0)
            throw std::runtime_error(server->getName() + " : negative max_body size");

        server->setClientMaxBodySize(max_body);
    } 
    catch (std::exception &e) {
        throw std::runtime_error(server->getName() + " : client_max_body_size : " + std::string(e.what()));
    }

    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(server->getName() + " : client_max_body_size should end with ;");

    tokens.erase(tokens.begin());

    std::cout << "client_max_body_size : " << server->getClientMaxBodySize() << std::endl;
}

void parse_error_page(HttpServer *server , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin());
    int error = -1; 
    try 
    {
        error = safe_atoi(tokens[0].second);
    }
    catch (std::exception &e)
    {
        throw std::runtime_error(server->getName() + " : Error page : " + std::string(e.what()));
    }

    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error(server->getName() + "Missing error path");

    server->setErrorPage(error, tokens[0].second);

    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(server->getName() + "Missing ; after error page");
    
    tokens.erase(tokens.begin());

    std::cout << "Error_page " << error << " " << server->getErrorPage(error) << std::endl;
    return;
}

void parse_location_root(Location *location , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin());
    
    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error(location->getSource() + " : Missing path for root");

    location->setRoot(tokens[0].second);

    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(location->getSource() + " : Missing ; after root");

    tokens.erase(tokens.begin());

    std::cout << location->getSource() << " | root : " << location->getRoot() << std::endl;
}

void parse_location_methods(Location *location , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin()); 

    try 
    {
        while (tokens[0].first == TOKEN_WORDS) {
            location->addAllowedMethod(tokens[0].second); 
            tokens.erase(tokens.begin());
        }
    }
    catch(std::exception &e)
    {
        throw std::runtime_error(location->getSource() + " : Allowed method : " + std::string(e.what()));
    }

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(location->getSource() + " : Missing semicolon on allowed method");
    
    tokens.erase(tokens.begin());
    
    std::cout   << location->getSource() 
                << " | GET " << location->isAllowedMethod("GET") 
                << " | POST " << location->isAllowedMethod("POST") 
                << " | DELETE " << location->isAllowedMethod("DELETE") << std::endl; 
    
    return;
}

void parse_location_index(Location *location , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin());
    
    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error(location->getSource() + " : Missing index");

    location->setIndex(tokens[0].second);
    
    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(location->getSource() + " : Missing semiconlon on index");

    tokens.erase(tokens.begin());

    std::cout << location->getSource() << " | index : " << location->getIndex() << std::endl;
}

void parse_location_cgi(Location *location , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin());
    
    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error(location->getSource() + " : Missing path in cgi");

    location->setCgiPath(tokens[0].second);
    
    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error(location->getSource() + " : Missing path in cgi");

    location->setCgiExt(tokens[0].second);
    
    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(location->getSource() + " : Missing semiconlon on cgi");

    tokens.erase(tokens.begin());

    std::cout << location->getSource() << " | cgi : " << location->getCgiPath() << " , "
        << location->getCgiExt() << std::endl;
}

void parse_location_autoindex(Location *location , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin());
    
    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error(location->getSource() + " : Missing value autoindex");

    if(tokens[0].second == "true")
        location->setAutoIndex(true);
    else if(tokens[0].second == "false")
        location->setAutoIndex(false);
    else    
        throw std::runtime_error(location->getSource() + " : Unkown autoindex value");

    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(location->getSource() + " : Missing semiconlon on autoindex");

    tokens.erase(tokens.begin());

    std::cout << location->getSource() << " | autoindex : " << location->getAutoIndex() << std::endl;
}

void parse_comment(std::vector<t_token> &tokens)
{
    if(tokens[0].first != TOKEN_COMMENT)
        return;

    tokens.erase(tokens.begin());

    while(tokens[0].first != TOKEN_RETURN)
    {
        if(tokens[0].first == TOKEN_EOF)
            break;

        tokens.erase(tokens.begin());
    }

    tokens.erase(tokens.begin());
}

void parse_location_upload(Location *location , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin());
    
    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error(location->getSource() + " : Missing upload path");

    location->setUploadPath(tokens[0].second);
    
    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(location->getSource() + " : Missing semiconlon on upload");

    tokens.erase(tokens.begin());

    std::cout << location->getSource() << " | upload : " << location->getUploadPath() << std::endl;
}

void parse_location_redirect(Location *location , std::vector<t_token> &tokens)
{
    tokens.erase(tokens.begin());
    
    if(tokens.size() < 2 || tokens[0].first != TOKEN_WORDS || tokens[1].first != TOKEN_WORDS)
        throw std::runtime_error(location->getSource() + " : redirect needs code and URL");

    int code = std::atoi(tokens[0].second.c_str());
    if (code != 301 && code != 302 && code != 303 && code != 307 && code != 308)
        throw std::runtime_error(location->getSource() + " : invalid redirect code");
        
    location->setRedirectCode(code);
    tokens.erase(tokens.begin());
    
    location->setRedirectUrl(tokens[0].second);
    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_SEMICOLON)
        throw std::runtime_error(location->getSource() + " : Missing semicolon on redirect");

    tokens.erase(tokens.begin());

    std::cout << location->getSource() << " | redirect : " << location->getRedirectCode() 
              << " " << location->getRedirectUrl() << std::endl;
}

void parse_location(HttpServer *server , std::vector<t_token> &tokens)
{
    Location location;
    
    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_WORDS)
        throw std::runtime_error("Missing source for location");

    location.setSource(tokens[0].second);
    tokens.erase(tokens.begin());

    if(tokens[0].first != TOKEN_LBRACE)
        throw std::runtime_error("Missig { before localisation");

    tokens.erase(tokens.begin());

    while (tokens.size() > 0) {
        TokenType type = tokens[0].first;
        std::string value = tokens[0].second;

        if(type == TOKEN_RBRACE || type == TOKEN_EOF)
            break;

        if(type == TOKEN_RETURN)
        {
            tokens.erase(tokens.begin());
            continue;
        } 

        switch (type) {
            case TOKEN_ROOT :
                parse_location_root(&location, tokens); break;
            case TOKEN_ALLOW_METHODS :
                parse_location_methods(&location, tokens); break;
            case TOKEN_INDEX:
                parse_location_index(&location, tokens); break;
            case TOKEN_CGI :
                parse_location_cgi(&location , tokens); break;
            case TOKEN_AUTOINDEX :
                parse_location_autoindex(&location , tokens); break;
            case TOKEN_COMMENT :
                parse_comment(tokens); break;
            case TOKEN_UPLOAD :
                parse_location_upload(&location , tokens) ; break;
            case TOKEN_REDIRECT :
                parse_location_redirect(&location , tokens) ; break;
            default:
                throw std::runtime_error("Unkown location directive " + value);
        } 
    }
   
    if(tokens[0].first != TOKEN_RBRACE)
        throw std::runtime_error("Missing } at the end of location");

    tokens.erase(tokens.begin());
    server->addLocation(location);
    return ; 
}


void parse_server(std::vector<HttpServer> &servers, std::vector<t_token> &tokens)
{

    PARSER_STATE state = NONE;

    while(!tokens.empty())
    {
        TokenType type      = tokens[0].first;
        std::string value   = tokens[0].second;

        if(type == TOKEN_RBRACE && state == BLOCK_SERVER)
            break;

        if(type == TOKEN_RETURN)
        {
            tokens.erase(tokens.begin());
            continue;
        }
         
        if(state == NONE)
        {
            if(type != TOKEN_WORDS)
                throw std::runtime_error("Special token should be inside a server");

            servers.push_back(HttpServer(-1 , value));
            state = BLOCK_SERVER_ENTER;

            tokens.erase(tokens.begin());
            continue;
        }

        if(state == BLOCK_SERVER_ENTER)
        {
            HttpServer server = servers.back();

            if(type != TOKEN_LBRACE)
                throw std::runtime_error(server.getName() + " : Block server should be surounded by {*}");
            
            tokens.erase(tokens.begin());
            state = BLOCK_SERVER;
            continue;
        }

        if(state == BLOCK_SERVER)
        {
            switch (type) 
            {
                case TOKEN_EOF :
                    throw std::runtime_error("Missing } on the server"); break;
                case TOKEN_LISTEN :
                    parse_listen(&servers.back(), tokens);break;
                case TOKEN_CLIENT_MAX_BODY_SIZE :
                    parse_client_max_body_size(&servers.back() , tokens); break;
                case TOKEN_ERROR_PAGE :
                    parse_error_page(&servers.back() , tokens); break;
                case TOKEN_LOCATION :
                    parse_location(&servers.back() , tokens); break;
                case TOKEN_COMMENT :
                    parse_comment(tokens) ; break;
                default :
                    throw std::runtime_error("Unkown configuration : " + value);
                
            if(type == TOKEN_RBRACE)
                state = NONE;
            } 
        }
    }
    
    if(tokens[0].first != TOKEN_RBRACE)
        throw std::runtime_error("Missing ; at the end of server");

    tokens.erase(tokens.begin());
}

std::vector<HttpServer> parser(std::string path)
{
    std::ifstream config(path.c_str());

    if(!config.is_open())
    {
        throw std::runtime_error("Can't open config file");
    }
    
    std::vector< t_token > tokens = tokenizer(config);

    std::vector<HttpServer> servers;
   
    while (tokens[0].first != TOKEN_EOF) {
        parse_comment(tokens);
        parse_server(servers, tokens);

        while (tokens[0].first == TOKEN_RETURN) {
            tokens.erase(tokens.begin());
        }
    } 

    return servers;
}
