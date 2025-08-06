/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/03 15:38:13 by caio              #+#    #+#             */
/*   Updated: 2025/08/05 21:04:51 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include "Utils.hpp"




class Client
{
    private:
        int _client_fd;
        sockaddr_in _client_addr;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        std::string _hostname;
        std::string _password;

        std::string _buffer;

    public:
        Client(int client_socket, sockaddr_in client_addr);
        ~Client();
        
        int getFd() const;
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getRealname() const;
        std::string getHostname() const;
        std::string getPassword() const;

        std::string getBuffer() const;

        void setNickname(const std::string &nickname);
        

        void appendBuffer(const std::string &data);

        void setNamesAndPass(const std::string &data);

        
        

        
    
};