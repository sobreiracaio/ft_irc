/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 16:58:52 by caio              #+#    #+#             */
/*   Updated: 2025/08/04 16:09:12 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>

#include "Utils.hpp"
#include "Client.hpp"

#define BUFFER_SIZE 4096

class Server
{
    private:
    
        int _port;
        std::string _password;
        int _server_fd;
        sockaddr_in _server_addr;
        std::vector<struct pollfd> _poll_fds;
        std::map<int, Client*> _clients;
        
    
        // Private initialization methods
        bool _checkPassword(void);
        bool _checkPort(void);
        int _createSocket(void);
        int _bindSocket(void);
        int _listenSocket(void);

        //Private management methods
        void _acceptNewClient(void);
        void _handleClientData(int client_fd);
        void _removeClient(int client_fd);
        
       
        
    
    public:
        Server(std::string port, std::string password);
        ~Server();

        //MAIN PUBLIC METHODS
        bool serverInit(void);
        void run(void);
        int getServerFd(void);
        
        //CLIENT MANAGEMENT METHODS
        Client *getClient(int client_fd);
        
};