/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/05 17:34:02 by caio              #+#    #+#             */
/*   Updated: 2025/08/08 16:18:38 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Utils.hpp"
#include "Client.hpp"
#include <vector>
#include <map>

class Channel
{
    private:
        std::string _name;
        std::string _topic;
        std::string _welcome_msg;  //MESSAGE OF THE DAY
        std::string _password;
        std::vector<std::string> _userlist;
        std::map<std::string, std::string> _registeredUsers; //<username, password>
        
    
    public:
        Channel(std::string const &name, std::string const &password);
        ~Channel();
        
        void setTopic(std::string const &topic);
        

        bool isPasswordRequired();

        void addUser(std::string const &user);

        std::string getName();
        std::string getTopic();
        std::string getWelcomeMsg();
        
        
    
    
};