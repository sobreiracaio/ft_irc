/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/05 17:34:02 by caio              #+#    #+#             */
/*   Updated: 2025/08/08 13:45:40 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Utils.hpp"
#include "Client.hpp"
#include <vector>


class Channel
{
    private:
        std::string _name;
        std::string _topic;
        std::string _welcome_msg;  //MESSAGE OF THE DAY
        std::string _password;
        std::vector<std::string> _userlist;
        
    
    public:
        Channel(std::string const &name, std::string const &password);
        ~Channel();
        
        void setName(std::string const &name);
        void setTopic(std::string const &topic);
        void setWelcomeMsg(std::string const &MOTD);

        std::string getName();
        std::string getTopic();
        std::string getWelcomeMsg();
        
        
    
    
};