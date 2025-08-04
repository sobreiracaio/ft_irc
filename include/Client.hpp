/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/03 15:38:13 by caio              #+#    #+#             */
/*   Updated: 2025/08/04 14:16:29 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

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

    public:
        Client(int client_socket, sockaddr_in client_addr);
        ~Client();
    
};