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

# include <iostream>
# include <cctype>
# include <sstream>
#include "HttpServer.hpp"

enum FILE_TYPE { BINARY , FOLDER , TEXT , HTML , CSS , JS , PNG , JPEG , ICO , MP4 , MPEG , ERR_DENIED , ERR_NOTFOUND }; 



bool        isUpper(std::string str);
bool        isValidProtocol(std::string str);
bool        isValidMethod(std::string str);
std::string intToString(int number);
FILE_TYPE   mime(const std::string& str);
std::string indexof(Location& location , std::string path);



#endif