/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/03 15:52:39 by caio              #+#    #+#             */
/*   Updated: 2025/08/18 14:50:33 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>

#define LOG 0
#define ERR 1

// Terminal Colors
#define WHITE   "\033[37m"
#define YELLOW  "\033[33m"
#define GREEN   "\033[32m"
#define BLUE    "\033[38;5;110m"
#define RED     "\033[31m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define RESET   "\033[0m"
#define BOLD    "\033[1m"

#define BUFFER_SIZE 4096
#define MAX_NICK_LENGTH 30      
#define MAX_CHANNEL_NAME 50
#define MAX_MESSAGE_LENGTH  512
#define MAX_TOPIC_LENGTH  307
#define MAX_CHANNELS_PER_USER  10
#define MAX_USERS_PER_CHANNEL  100

// log generation functions
void logMessage(std::string msg, std::string msg_color, std::string args, std::string args_color, int type = LOG);


// Conversions and validation
std::string itoa(int number);
bool isNum(const std::string &str);
bool isValidPort(int port);
bool isValidNickChar(char c);
bool isValidChannelChar(char c);


// IRC specific manipulations
bool isValidNickname(const std::string &nickname);
bool isValidChannelName(const std::string &channelName);









