/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 14:16:32 by caio              #+#    #+#             */
/*   Updated: 2025/08/05 19:01:18 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"

Client::Client(int client_socket, sockaddr_in client_addr):_client_fd(client_socket), _client_addr(client_addr)
{
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET6_ADDRSTRLEN);
    this->_hostname = std::string(ip_str);

    logMessage("New client connected! FD= ", BLUE, itoa(client_socket), GREEN);
}

Client::~Client(){}


int Client::getFd() const
{
    return this->_client_fd;
}

std::string Client::getHostname() const
{
    return this->_hostname;
}

std::string Client::getNickname() const
{
    return this->_nickname;
}

std::string Client::getRealname() const
{
    return this->_realname;
}

std::string Client::getUsername() const
{
    return this->_username;
}

std::string Client::getPassword() const
{
    return this->_password;
}

std::string Client::getBuffer() const
{
    return this->_buffer;
}

void Client::setNickname(const std::string &nickname)
{
    this->_nickname = nickname;
}

void Client::setRealname(const std::string &realname)
{
    this->_realname = realname;
}

void Client::setUsername(const std::string &username)
{
    this->_username = username;
}

void Client::appendBuffer(const std::string &data)
{
    this->_buffer += data;
}

void Client::setNamesAndPass(std::string const &data)
{
    std::istringstream iss(data);
    std::string line;

    while(std::getline(iss, line, '\n'))
    {
        if(!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);
        
        std::string cmd = line.substr(0, 5);
        
        
        if(cmd == "PASS ")
            this->_password = line.substr(5);
        if(cmd == "NICK ")
            this->setNickname(line.substr(5));
        if(cmd == "USER ")
        {
            std::istringstream temp_iss(line.substr(5));
            std::string temp;
            std::getline(temp_iss, temp, ' ');
            this->setUsername(temp);
            std::getline(temp_iss, temp, '\n');
            this->setRealname(temp.substr(5));
            
        }
    }
}