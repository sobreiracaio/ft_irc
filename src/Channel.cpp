/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/05 17:34:25 by caio              #+#    #+#             */
/*   Updated: 2025/08/11 19:14:57 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Channel.hpp"

Channel::Channel(std::string const &name, std::string const &password):_name(name), _password(password)
{
    
}

Channel::~Channel(){}


void Channel::setTopic(std::string const &topic)
{
    
}

bool Channel::isPasswordRequired()
{
    return false;
}


void Channel::addUser(std::string const &user)
{
   
    
    this->_userlist.insert(user);
    
}

std::string Channel::getName()
{
    return this->_name;
}

 void Channel::connect(Server *server, int client_fd)
 {
    Client *client = server->getClient(client_fd);
    
    std::string server_name = client->getHostname();
    std::string client_nick = client->getNickname();
    std::string client_user = client->getUsername();
    std::string channel_name = this->getName();
    std::string user_list;

    std::string joinConfirm = ":" + client_nick + "!" + client_user + "@" + server_name +
                              " JOIN :" + channel_name + "\r\n";
    send(client_fd, joinConfirm.c_str(), joinConfirm.length(), 0);

    std::set<std::string>::iterator it;
    
   for(it = this->_userlist.begin(); it != this->_userlist.end(); it++)
    {
        if(it != this->_userlist.begin())
            user_list.append(" ");
        user_list.append(*it);
    }
    std::string list_of_users = ":" + server_name + " 353 " + client_nick + " = " + channel_name + " :" +
                                user_list + "\r\n";
    

    for(it = this->_userlist.begin(); it != this->_userlist.end(); it++)
    {
        int fd = server->getClientByNick(*it)->getFd();
        send(fd, list_of_users.c_str(), list_of_users.length(), 0);
    }                            
    
    std::string end_names = ":" + server_name + " 366 " + client_nick + " " + channel_name +
                            " :End of /NAMES list." + "\r\n";          
    
    

 }