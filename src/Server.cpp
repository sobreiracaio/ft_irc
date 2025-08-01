/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/07/31 17:46:14 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(std::string port, std::string password)
{
    if (checkPort())
        this->_port = std::atoi(port.c_str());
    if(checkPassword())
        this->_password = password;
}

Server::~Server()
{
    close(this->_socket_fd);
}

int Server::createSocket()
{
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_socket == -1)
    {
        std::cerr << "Cant create socket!" << std::endl;
        return -1;
    }
    std::cout << "Server socket created successfully!" << std::endl;
    this->_socket_fd = listen_socket;
    return (this->_socket_fd);
}

int Server::bindSocket(void)
{
    
    this->_server_addr.sin_family = AF_INET;
    this->_server_addr.sin_port = htons(this->_port);
    inet_pton(AF_INET, "0.0.0.0", &this->_server_addr.sin_addr);
 
    if(bind(this->_socket_fd, (struct sockaddr*)&this->_server_addr, sizeof(this->_server_addr)) == -1)
    {
        std::cerr << "Cant bind to IP/port" << std::endl;
        return -1;
    }
    std::cout <<"Bind successful!"<< std::endl;
    
    return (0);
}

int Server::listenSocket()
{
    if(listen(this->_socket_fd, SOMAXCONN) == -1)
    {
        std::cerr <<"Cant listen!" << std::endl;
        return -1;
    }
    else
    {
        std::cout << "Waiting for connections..." << std::endl;
    }
    return (0);
}

bool Server::checkPassword()
{
    return true;
}

bool Server::checkPort()
{
    return true;
}