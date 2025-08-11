/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/05 17:34:02 by caio              #+#    #+#             */
/*   Updated: 2025/08/11 18:47:41 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Utils.hpp"
//#include "Client.hpp"
#include "Server.hpp"
#include <vector>
#include <map>
#include <set>

class Server;
class Client;

class Channel
{
    private:
        std::string _name;
        std::string _topic;
        std::string _welcome_msg;  //MESSAGE OF THE DAY
        std::string _password;
        std::set<std::string> _userlist;
        
        
    
    public:
        Channel(std::string const &name, std::string const &password);
        ~Channel();
        
        void setTopic(std::string const &topic);
        
        bool isPasswordRequired();

        void addUser(std::string const &user);

        std::string getName();
        std::string getTopic();
        std::string getWelcomeMsg();

        void connect(Server *server, int client_fd);
        
        
    
    
};