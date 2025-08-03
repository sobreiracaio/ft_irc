/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 16:58:52 by caio              #+#    #+#             */
/*   Updated: 2025/08/03 16:17:20 by caio             ###   ########.fr       */
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
#include <string>
#include <cstring>
#include <cstdlib>

#include "Utils.hpp"

class Server
{
    private:
        int _port;
        std::string _password;
        int _server_fd;
        sockaddr_in _server_addr;
        bool _checkPassword(void);
        bool _checkPort(void);
        int _createSocket(void);
        int _bindSocket(void);
        int _listenSocket(void);
        
    
    public:
        Server(std::string port, std::string password);
        ~Server();
        bool serverInit(void);
        int getServerFd(void);
};