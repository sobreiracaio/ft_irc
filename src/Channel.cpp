/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/05 17:34:25 by caio              #+#    #+#             */
/*   Updated: 2025/08/08 17:08:40 by caio             ###   ########.fr       */
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
    //check for doubles -> se tiver duplicata chama funcao de doubles e adiciona um "_" no nick
    
    this->_userlist.push_back(user);
    
}

std::string Channel::getName()
{
    return this->_name;
}