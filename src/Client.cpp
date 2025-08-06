/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 14:16:32 by caio              #+#    #+#             */
/*   Updated: 2025/08/05 22:05:02 by caio             ###   ########.fr       */
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
    static bool flag = false;
    
    if(flag)
    {
        std::string msg = ":" + this->getNickname() + "!" + this->getUsername() + "@" + this->getHostname() + " " +
                            "NICK :" + nickname + "\r\n";
        
        std::cout <<"FD = "<< this->getFd() << " --> " << msg << std::endl;                    
        send(this->getFd(), msg.c_str(), msg.length(), 0);
    }
    flag = true;
    this->_nickname = nickname;
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
            this->_username = temp;
            std::getline(temp_iss, temp, '\n');
            this->_realname = temp.substr(5);
            
        }
    }
}