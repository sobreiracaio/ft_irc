/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/05 17:34:02 by caio              #+#    #+#             */
/*   Updated: 2025/08/05 18:32:05 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Utils.hpp"
#include "Client.hpp"


class Channel
{
    private:
        std::string _name;
        std::string _topic;
        std::string _motd;  //MESSAGE OF THE DAY
        
    
    public:
        Channel();
        ~Channel();
        
        void setName(std::string const &name);
        void setTopic(std::string const &topic);
        void setMOTD(std::string const &MOTD);

        std::string getName();
        std::string getTopic();
        std::string getMOTD();
        
        
    
    
};