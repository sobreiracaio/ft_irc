/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/08/03 16:19:53 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(std::string port, std::string password)
{
    if (_checkPort())
        this->_port = atoi(port.c_str());
    if(_checkPassword())
        this->_password = password;
}

Server::~Server()
{
    close(this->_server_fd);
}

int Server::_createSocket()
{
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_socket == -1)
    {
        logMessage("ERROR: ", RED, "Can't create socket!", YELLOW, ERR);
        return -1;
    }
    
    if (fcntl(listen_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        logMessage("ERROR: ", RED, "Failed to set server socket to non-blocking mode!", YELLOW, ERR);
        close(listen_socket);
        return -1;
    }
    
    int opt = 1;
    setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    logMessage("Server socket created successfully!", BLUE, "", RESET);
    this->_server_fd = listen_socket;
    return (this->_server_fd);
}

int Server::_bindSocket(void)
{
    memset(&this->_server_addr, 0, sizeof(this->_server_addr));
    this->_server_addr.sin_family = AF_INET;
    this->_server_addr.sin_port = htons(this->_port);
    inet_pton(AF_INET, "0.0.0.0", &this->_server_addr.sin_addr);
 
    if(bind(this->_server_fd, (struct sockaddr*)&this->_server_addr, sizeof(this->_server_addr)) == -1)
    {
        logMessage("ERROR: ", RED, "Can't bind to IP/port!", YELLOW, ERR);
        return -1;
    }
    logMessage("Bind successful!", BLUE, "", RESET);
    
    return (0);
}

int Server::_listenSocket()
{
    if(listen(this->_server_fd, SOMAXCONN) == -1)
    {
        logMessage("ERROR: ", RED, "Can't listen!", YELLOW, ERR);
        return -1;
    }
    else
    {
        logMessage("Waiting for connections...", GREEN, "", RESET);
    }
    return (0);
}

bool Server::serverInit()
{
    return(this->_createSocket() != -1 &&
            this->_bindSocket() != -1 &&
            this->_listenSocket() != -1);
}

int Server::getServerFd(void)
{
    return this->_server_fd;
}

bool Server::_checkPassword()
{
    return true;
}

bool Server::_checkPort()
{
    return true;
}