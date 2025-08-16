/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsampiet <lsampiet@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/03 15:37:55 by caio              #+#    #+#             */
/*   Updated: 2025/08/16 18:35:17 by lsampiet         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Utils.hpp"

// log generation functions
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

void logError(const std::string &function, const std::string &error)
{
    std::cerr << RED << "[ERROR] " << function << ": " << error << RESET << std::endl;
}

void logWarning(const std::string &message)
{
    std::cout << YELLOW << "[WARNING] " << message << RESET << std::endl;
}

void logInfo(const std::string &message)
{
    std::cout << BLUE << "[INFO] " << message << RESET << std::endl;
}

void logDebug(const std::string &message)
{
    std::cout << CYAN << "[DEBUG] " << message << RESET << std::endl;
}

// Conversions and validation
std::string itoa(int number)
{
    std::ostringstream oss;
    oss << number;
    return oss.str();
}

bool isNum(const std::string &str)
{
    if (str.empty())
        return false;
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+')
        start = 1;
    
    if (start >= str.length())
        return false;
    
    for (size_t i = start; i < str.length(); i++)
    {
        if (!isdigit(str[i]))
            return false;
    }
    return true;
}

bool isValidPort(int port)
{
    return (port > 0 && port <= 65535);
}

bool isValidNickChar(char c)
{
    return (isalnum(c) || c == '[' || c == ']' || c == '\\' || 
            c == '`' || c == '_' || c == '^' || c == '{' || c == '}' || c == '-');
}

bool isValidChannelChar(char c)
{
    return (c != ' ' && c != ',' && c != '\a' && c != '\0' && 
            c != '\r' && c != '\n' && c != ':');
}

// String manipulation
std::string trim(const std::string &str)
{
    std::string trimmed = str;
    
    // Remove leading whitespace
    size_t start = trimmed.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    
    trimmed = trimmed.substr(start);
    
    // Remove trailing whitespace
    size_t end = trimmed.find_last_not_of(" \t\r\n");
    if (end != std::string::npos)
        trimmed = trimmed.substr(0, end + 1);
    
    return trimmed;
}

std::string toUpper(const std::string &str)
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); i++)
        result[i] = std::toupper(result[i]);
    return result;
}

std::string toLower(const std::string &str)
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); i++)
        result[i] = std::tolower(result[i]);
    return result;
}

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter))
    {
        if (!item.empty())
            result.push_back(item);
    }
    
    return result;
}

std::vector<std::string> split(const std::string &str, const std::string &delimiter)
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = 0;
    
    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        std::string token = str.substr(start, end - start);
        if (!token.empty())
            result.push_back(token);
        start = end + delimiter.length();
    }
    
    std::string last_token = str.substr(start);
    if (!last_token.empty())
        result.push_back(last_token);
    
    return result;
}

std::string join(const std::vector<std::string> &vec, const std::string &delimiter)
{
    std::string result;
    for (size_t i = 0; i < vec.size(); i++)
    {
        result += vec[i];
        if (i < vec.size() - 1)
            result += delimiter;
    }
    return result;
}

bool startsWith(const std::string &str, const std::string &prefix)
{
    if (prefix.length() > str.length())
        return false;
    
    return str.substr(0, prefix.length()) == prefix;
}

bool endsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.length() > str.length())
        return false;
    
    return str.substr(str.length() - suffix.length()) == suffix;
}

// IRC specific validations
bool isValidNickname(const std::string &nickname)
{
    if (nickname.empty() || nickname.length() > IRC::MAX_NICK_LENGTH)
        return false;
    
    // First character must be letter or special IRC char
    char first = nickname[0];
    if (!isalpha(first) && first != '[' && first != ']' && first != '\\' && 
        first != '`' && first != '_' && first != '^' && first != '{' && first != '}')
        return false;
    
    // Other characters
    for (size_t i = 1; i < nickname.length(); i++)
    {
        if (!isValidNickChar(nickname[i]))
            return false;
    }
    
    return true;
}

bool isValidChannelName(const std::string &channelName)
{
    if (channelName.empty() || channelName.length() > IRC::MAX_CHANNEL_NAME)
        return false;
    
    // Channel must start with # or &
    if (channelName[0] != '#' && channelName[0] != '&')
        return false;
    
    // Check remaining characters
    for (size_t i = 1; i < channelName.length(); i++)
    {
        if (!isValidChannelChar(channelName[i]))
            return false;
    }
    
    return true;
}

bool isValidHostname(const std::string &hostname)
{
    if (hostname.empty() || hostname.length() > 253)
        return false;
    
    // Basic hostname validation
    for (size_t i = 0; i < hostname.length(); i++)
    {
        char c = hostname[i];
        if (!isalnum(c) && c != '.' && c != '-')
            return false;
    }
    
    return true;
}

bool hasIllegalChars(const std::string &str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        unsigned char c = static_cast<unsigned char>(str[i]);
        
        // Check for illegal control characters
        if (c < 32 && c != '\t' && c != '\r' && c != '\n')
            return true;
        
        // Check for null byte
        if (c == 0)
            return true;
    }
    return false;
}

// Time formatting
std::string getCurrentTime()
{
    time_t now = time(0);
    struct tm *timeinfo = localtime(&now);
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << timeinfo->tm_hour << ":"
        << std::setfill('0') << std::setw(2) << timeinfo->tm_min << ":"
        << std::setfill('0') << std::setw(2) << timeinfo->tm_sec;
    
    return oss.str();
}

std::string formatTime(time_t timestamp)
{
    struct tm *timeinfo = localtime(&timestamp);
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << timeinfo->tm_mday << "/"
        << std::setfill('0') << std::setw(2) << (timeinfo->tm_mon + 1) << "/"
        << (timeinfo->tm_year + 1900) << " "
        << std::setfill('0') << std::setw(2) << timeinfo->tm_hour << ":"
        << std::setfill('0') << std::setw(2) << timeinfo->tm_min << ":"
        << std::setfill('0') << std::setw(2) << timeinfo->tm_sec;
    
    return oss.str();
}

std::string getUptime(time_t start_time)
{
    time_t now = time(0);
    time_t uptime = now - start_time;
    
    int days = uptime / 86400;
    int hours = (uptime % 86400) / 3600;
    int minutes = (uptime % 3600) / 60;
    int seconds = uptime % 60;
    
    std::ostringstream oss;
    if (days > 0)
        oss << days << " days, ";
    if (hours > 0)
        oss << hours << " hours, ";
    if (minutes > 0)
        oss << minutes << " minutes, ";
    oss << seconds << " seconds";
    
    return oss.str();
}

// IRC message formatting
std::string formatIRCMessage(const std::string &prefix, const std::string &command, 
                            const std::vector<std::string> &params)
{
    std::string message;
    
    if (!prefix.empty())
        message += ":" + prefix + " ";
    
    message += command;
    
    for (size_t i = 0; i < params.size(); i++)
    {
        message += " ";
        
        // Last parameter with spaces needs ":"
        if (i == params.size() - 1 && params[i].find(' ') != std::string::npos)
            message += ":";
        
        message += params[i];
    }
    
    message += "\r\n";
    return message;
}

std::string formatNumericReply(const std::string &server, int code, const std::string &target,
                              const std::string &message)
{
    std::ostringstream oss;
    oss << ":" << server << " " << std::setfill('0') << std::setw(3) << code 
        << " " << target << " " << message << "\r\n";
    
    return oss.str();
}

// Network utilities
std::string getLocalIP()
{
    // Simplified implementation - returns localhost
    return "127.0.0.1";
}

bool isValidIP(const std::string &ip)
{
    std::vector<std::string> parts = split(ip, '.');
    
    if (parts.size() != 4)
        return false;
    
    for (size_t i = 0; i < parts.size(); i++)
    {
        if (!isNum(parts[i]))
            return false;
        
        int num = atoi(parts[i].c_str());
        if (num < 0 || num > 255)
            return false;
    }
    
    return true;
}

// Safety and sanitizing
std::string sanitizeInput(const std::string &input)
{
    std::string sanitized;
    
    for (size_t i = 0; i < input.length(); i++)
    {
        unsigned char c = static_cast<unsigned char>(input[i]);
        
        // Allow printable characters and some control chars
        if (c >= 32 || c == '\t' || c == '\r' || c == '\n')
            sanitized += c;
    }
    
    return sanitized;
}

std::string escapeSpecialChars(const std::string &str)
{
    std::string escaped;
    
    for (size_t i = 0; i < str.length(); i++)
    {
        char c = str[i];
        
        switch (c)
        {
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            default:
                escaped += c;
                break;
        }
    }
    
    return escaped;
}

bool isSafeString(const std::string &str)
{
    // Check for dangerous patterns or excessive length
    if (str.length() > IRC::MAX_MESSAGE_LENGTH)
        return false;
    
    if (hasIllegalChars(str))
        return false;
    
    // Check for common injection patterns (basic check)
    std::string lower = toLower(str);
    if (lower.find("script") != std::string::npos ||
        lower.find("eval") != std::string::npos ||
        lower.find("exec") != std::string::npos)
        return false;
    
    return true;
}
