/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/03 15:52:39 by caio              #+#    #+#             */
/*   Updated: 2025/08/04 17:07:19 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <string>
#include <sstream>

#define LOG 0
#define ERR 1

#define WHITE   "\033[37m"
#define YELLOW  "\033[33m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define RED     "\033[31m"
#define RESET   "\033[0m"

void logMessage(std::string msg, std::string msg_color, std::string args, std::string args_color, int type = LOG);
std::string itoa(int number);
bool isNum(const std::string &str);