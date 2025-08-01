/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 16:58:52 by caio              #+#    #+#             */
/*   Updated: 2025/07/31 17:43:45 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>

class Server
{
    private:
    int _port;
    std::string _password;
    int _socket_fd;
    sockaddr_in _server_addr;
    bool checkPassword(void);
    bool checkPort(void);
        
    
    public:
        Server(std::string port, std::string password);
        ~Server();
        int createSocket(void);
        int bindSocket(void);
        int listenSocket(void);
        
};