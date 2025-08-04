/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/03 15:37:55 by caio              #+#    #+#             */
/*   Updated: 2025/08/03 21:20:46 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Utils.hpp"


void logMessage(std::string msg, std::string msg_color, std::string args, std::string args_color, int type)
{
    if (type == ERR)
    {
        std::cerr << msg_color << msg << RESET;
        std::cerr << args_color << args << RESET << std::endl;
    }
    else
    {
        std::cout << msg_color << msg << RESET;
        std::cout << args_color << args << RESET << std::endl;
    }
}

std::string itoa(int number)
{
    std::ostringstream oss;
    oss << number;
    std::string str = oss.str();
    return (str);
}