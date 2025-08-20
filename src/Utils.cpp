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

// IRC specific validations
bool isValidNickname(const std::string &nickname)
{
    if (nickname.empty() || nickname.length() > MAX_NICK_LENGTH)
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
    if (channelName.empty() || channelName.length() > MAX_CHANNEL_NAME)
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

