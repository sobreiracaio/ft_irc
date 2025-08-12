/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/03 15:52:39 by caio              #+#    #+#             */
/*   Updated: 2025/08/12 20:02:55 by caio             ###   ########.fr       */
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

// log generation functions
void logMessage(std::string msg, std::string msg_color, std::string args, std::string args_color, int type = LOG);
void logError(const std::string &function, const std::string &error);
void logWarning(const std::string &message);
void logInfo(const std::string &message);
void logDebug(const std::string &message);

// Conversions and validation
std::string itoa(int number);
bool isNum(const std::string &str);
bool isValidPort(int port);
bool isValidNickChar(char c);
bool isValidChannelChar(char c);

// Strings manipulation
std::string trim(const std::string &str);
std::string toUpper(const std::string &str);
std::string toLower(const std::string &str);
std::vector<std::string> split(const std::string &str, char delimiter);
std::vector<std::string> split(const std::string &str, const std::string &delimiter);
std::string join(const std::vector<std::string> &vec, const std::string &delimiter);
bool startsWith(const std::string &str, const std::string &prefix);
bool endsWith(const std::string &str, const std::string &suffix);

// IRC specific manipulations
bool isValidNickname(const std::string &nickname);
bool isValidChannelName(const std::string &channelName);
bool isValidHostname(const std::string &hostname);
bool hasIllegalChars(const std::string &str);

// Time formatting
std::string getCurrentTime();
std::string formatTime(time_t timestamp);
std::string getUptime(time_t start_time);

// IRC message formatting
std::string formatIRCMessage(const std::string &prefix, const std::string &command, 
                            const std::vector<std::string> &params);
std::string formatNumericReply(const std::string &server, int code, const std::string &target,
                              const std::string &message);

// Net utils
std::string getLocalIP();
bool isValidIP(const std::string &ip);

// Safety and sanitizing
std::string sanitizeInput(const std::string &input);
std::string escapeSpecialChars(const std::string &str);
bool isSafeString(const std::string &str);

// Constantes IRC
namespace IRC
{
    const int MAX_NICK_LENGTH = 30;
    const int MAX_CHANNEL_NAME = 50;
    const int MAX_MESSAGE_LENGTH = 512;
    const int MAX_TOPIC_LENGTH = 307;
    const int MAX_CHANNELS_PER_USER = 10;
    const int MAX_USERS_PER_CHANNEL = 100;
}