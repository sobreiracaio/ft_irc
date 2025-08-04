/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/08/03 21:34:44 by caio             ###   ########.fr       */
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
    if(this->_createSocket() == -1 ||
            this->_bindSocket() == -1 ||
            this->_listenSocket() == -1)
        return (false);
    
    
    struct pollfd server_pollfd;
    server_pollfd.fd = this->_server_fd;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    this->_poll_fds.push_back(server_pollfd);

    return (true);
}

void Server::run()
{
    char buffer[BUFFER_SIZE];
    
    while(true)
    {
        int poll_count = poll(&this->_poll_fds[0], this->_poll_fds.size(), -1);

        if (poll_count == -1)
        {
            logMessage("ERROR: ", RED, "Poll failed!", YELLOW, ERR);
            break;
        }

        size_t i;
        for (i = 0; i < this->_poll_fds.size(); i++)
        {
            if(this->_poll_fds[i].revents & POLLIN)
            {
                if(this->_poll_fds[i].fd == this->_server_fd) //NEW CONNECTION -> CREATE AN FUNCTION TO ACCEPT CLIENT
                {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_socket = accept(this->_server_fd, (struct sockaddr *)&client_addr, &client_len);
                    
                    if (client_socket < 0)
                    {
                        logMessage("ERROR: ", RED, "Accept failed!", YELLOW, ERR);
                        continue;
                    }
                    fcntl(client_socket, F_SETFL, O_NONBLOCK);
                    logMessage("New client connected! FD= ", BLUE, itoa(client_socket), GREEN);
                    struct pollfd client_pollfd;
                    client_pollfd.fd = client_socket;
                    client_pollfd.events = POLLIN;
                    client_pollfd.revents = 0;
                    this->_poll_fds.push_back(client_pollfd);
                }                                          // FUNCTION ACCEPT CLIENT ENDS HERE
                else
                {
                    //EXISTING CLIENT SENT SOMETHING -> HANDLE CLIENT DATA
                    memset(buffer, 0, BUFFER_SIZE);
                    int bytes_received = recv(this->_poll_fds[i].fd, buffer, BUFFER_SIZE, 0);
                    if (bytes_received <= 0)
                    {
                        std::cout << "Client disconnected!" << std::endl;
                        close(this->_poll_fds[i].fd);
                        this->_poll_fds.erase(this->_poll_fds.begin() + i);
                        --i; //INDEX CORRECTION AFTER ERASING
                    }
                    else
                    {
                        std::string msg(buffer, bytes_received);
                        std::cout << "Received from FD= " << this->_poll_fds[i].fd << ": " << msg;

                        //RESEND MESAGE TO CLIENT (FOR NOW ONLY FOR DEBUGGING PURPOSES)
                        msg += "\r\n"; //IRC FORMAT
                        std::string msg2 = "Resend: " + msg + "\r\n";
                        send(this->_poll_fds[i].fd, msg2.c_str(), msg.length(), 0); 
                    } // HANDLE CLIENT DATA ENDS HERE
                }
            }
        }
    }
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