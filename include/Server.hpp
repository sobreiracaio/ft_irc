/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 16:58:52 by caio              #+#    #+#             */
/*   Updated: 2025/08/06 16:22:40 by caio             ###   ########.fr       */
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
#include "Channel.hpp"

#define BUFFER_SIZE 4096

#define JOIN 100
#define PRIVMSG 101
#define NICK 102
#define QUIT 103
#define NO_COMM -1

class Server
{
    private:
    
        int _port;
        std::string _password;
        int _server_fd;
        sockaddr_in _server_addr;
        std::vector<struct pollfd> _poll_fds;
        std::map<int, Client*> _clients;
        std::map<std::string, Channel*> _channels;
        
        
    
        // Private initialization methods
        bool _checkPassword(std::string const &client_pass);
        int _createSocket(void);
        int _bindSocket(void);
        int _listenSocket(void);

        //Private management methods
        void _acceptNewClient(void);
        void _handleClientData(int client_fd);
        void _removeClient(int client_fd);
        
       
        
    
    public:
        Server(int port, std::string password);
        ~Server();

        //MAIN PUBLIC METHODS
        bool serverInit(void);
        void run(void);
        int getServerFd(void);
        
        //CLIENT MANAGEMENT METHODS
        Client *getClient(int client_fd);
        void changeNick(std::string const &data, int client_fd);
        

        //SERVER COMMANDS METHODS
        int parseCommand(const std::string& data);
        void executeCommand(int client_fd, int command_code, std::string const &data);
        
};