/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hrasoamb < hrasoamb@student.42antananar    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 11:07:08 by hrasoamb          #+#    #+#             */
/*   Updated: 2025/12/18 09:58:40 by hrasoamb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

#include "HttpServer.hpp"

enum FILE_TYPE { BINARY , FOLDER , TEXT , HTML , CSS , JS , PNG , JPEG , ICO , MP4 , MPEG , ERR_DENIED , ERR_NOTFOUND }; 

bool        isUpper(std::string str);
bool        isValidProtocol(std::string str);
bool        isValidMethod(std::string str);
bool        isValidPath(const std::string &path);
bool        isValidHeaderName(const std::string &name);
bool        isValidHeaderValue(const std::string &value);
bool        hasControlChars(const std::string &str);
std::string urlDecode(const std::string &str);
std::string intToString(int number);
FILE_TYPE   mime(const std::string& str);
std::string replaceFirstOccurrence(const std::string &originalString, const std::string &target, const std::string &replacement);
std::string indexof(Location& location, std::string path, std::string urlPath);
std::string itos(int n);
bool endsWith(const std::string& str, const std::string& suffix);

#endif
