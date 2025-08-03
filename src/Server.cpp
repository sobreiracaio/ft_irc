/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/08/03 15:32:35 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(std::string port, std::string password)
{
    if (checkPort())
        this->_port = atoi(port.c_str());
    if(checkPassword())
        this->_password = password;
}

Server::~Server()
{
    close(this->_server_fd);
}

int Server::createSocket()
{
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_socket == -1)
    {
        std::cerr << "Can't create socket!" << std::endl;
        return -1;
    }
    
    if (fcntl(listen_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set server socket to non-blocking mode!" << std::endl;
        close(listen_socket);
        return -1;
    }
    
    int opt = 1;
    setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    std::cout << "Server socket created successfully!" << std::endl;
    this->_server_fd = listen_socket;
    return (this->_server_fd);
}

int Server::bindSocket(void)
{
    memset(&this->_server_addr, 0, sizeof(this->_server_addr));
    this->_server_addr.sin_family = AF_INET;
    this->_server_addr.sin_port = htons(this->_port);
    inet_pton(AF_INET, "0.0.0.0", &this->_server_addr.sin_addr);
 
    if(bind(this->_server_fd, (struct sockaddr*)&this->_server_addr, sizeof(this->_server_addr)) == -1)
    {
        std::cerr << "Can't bind to IP/port!" << std::endl;
        return -1;
    }
    std::cout <<"Bind successful!"<< std::endl;
    
    return (0);
}

int Server::listenSocket()
{
    if(listen(this->_server_fd, SOMAXCONN) == -1)
    {
        std::cerr <<"Can't listen!" << std::endl;
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