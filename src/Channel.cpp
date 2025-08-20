#include "../include/Channel.hpp"

Channel::Channel(std::string const &name, std::string const &password)
    : _name(name), _password(password), _key(""), _user_limit(0)
{
    this->_creation_time = time(NULL);
    
    // Default modes for new channels
    this->_modes.insert(MODE_NO_EXTERNAL_MSGS); // +n by default
    this->_modes.insert(MODE_TOPIC_PROTECT);    // +t by default
    
    logMessage("Channel created: ", GREEN, name, BLUE);
}

Channel::~Channel()
{
    logMessage("Channel destroyed: ", RED,this->_name, YELLOW);
}

// Getters
std::string Channel::getName() const
{
    return this->_name;
}

std::string Channel::getTopic() const
{
    return this->_topic;
}

std::string Channel::getPassword() const
{
    return this->_password;
}

std::string Channel::getKey() const
{
    return this->_key;
}

size_t Channel::getUserCount() const
{
    return this->_userlist.size();
}

size_t Channel::getUserLimit() const
{
    return this->_user_limit;
}

time_t Channel::getCreationTime() const
{
    return this->_creation_time;
}

// Setters
void Channel::setTopic(std::string const &topic)
{
    this->_topic = topic;
    logMessage("Topic set for channel ", BLUE,this->_name + ": " + topic, GREEN);
}





// User management
bool Channel::addUser(std::string const &user, std::string const &password)
{
    // Remove espaços e caracteres de controle do nome do usuário
    std::string cleanUser = user;
    size_t pos = cleanUser.find_first_of(" \r\n\t");
    if (pos != std::string::npos)
        cleanUser.erase(pos);
    
    if (cleanUser.empty())
        return false;
    
    if (this->_userlist.find(cleanUser) != this->_userlist.end())
        return true; // Usuário já estava no canal - não é erro
    
    if (!canUserJoin(cleanUser, password))
        return false;
    
    this->_userlist.insert(cleanUser);
    
    // First user becomes operator
    if (this->_userlist.size() == 1)
        this->_operators.insert(cleanUser);
    
    // Remove from invite list if present
    this->_invited.erase(cleanUser);
    
    logMessage("User joined channel ", GREEN, this->_name + ": " + cleanUser, BLUE);
    return true;
}

bool Channel::removeUser(std::string const &user)
{
    if (this->_userlist.find(user) == this->_userlist.end())
        return false;
    
    this->_userlist.erase(user);
    this->_operators.erase(user);
    
    logMessage("User left channel ", YELLOW,this->_name + ": " + user, BLUE);
    return true;
}

bool Channel::hasUser(std::string const &user) const
{
    return (this->_userlist.find(user) != this->_userlist.end());
}

std::vector<std::string> Channel::getUserList() const
{
    std::vector<std::string> users;
    for (std::set<std::string>::const_iterator it = this->_userlist.begin(); it != this->_userlist.end(); ++it)
        users.push_back(*it);
    return users;
}

std::string Channel::getUserListString() const
{
    std::string result;
    for (std::set<std::string>::const_iterator it = this->_userlist.begin(); it != this->_userlist.end(); ++it)
    {
        if (!result.empty())
            result += " ";
        result += this->_getUserModePrefix(*it) + *it;
    }
    return result;
}

// Operators management
bool Channel::addOp(std::string const &user)
{
    if (!hasUser(user))
        return false;
    
    this->_operators.insert(user);
    logMessage("User became operator in ", GREEN,this->_name + ": " + user, BLUE);
    return true;
}

bool Channel::removeOp(std::string const &user)
{
    if (this->_operators.find(user) == this->_operators.end())
        return false;
    
    this->_operators.erase(user);
    logMessage("User lost operator in ", YELLOW,this->_name + ": " + user, BLUE);
    return true;
}

bool Channel::isOp(std::string const &user) const
{
    return (this->_operators.find(user) != this->_operators.end());
}

// Invite system
bool Channel::inviteUser(std::string const &user)
{
    this->_invited.insert(user);
    logMessage("User invited to ", GREEN,this->_name + ": " + user, BLUE);
    return true;
}

bool Channel::isInvited(std::string const &user) const
{
    return (this->_invited.find(user) != this->_invited.end());
}

// Modes management
bool Channel::setMode(char mode, bool enable, std::string const &param)
{
    if (enable)
    {
        this->_modes.insert(mode);
        if (mode == MODE_KEY)
            this->_key = param;
        else if (mode == MODE_LIMIT && !param.empty())
            this->_user_limit = atoi(param.c_str());
    }
    else
    {
        this->_modes.erase(mode);
        if (mode == MODE_KEY)
            this->_key.clear();
        else if (mode == MODE_LIMIT)
            this->_user_limit = 0;
    }
    
    logMessage("Mode change in ", BLUE,this->_name + ": " + (enable ? "+" : "-") + mode, GREEN);
    return true;
}

bool Channel::hasMode(char mode) const
{
    return this->_hasMode(mode);
}

std::string Channel::getModeString() const
{
    std::string modes = "+";
    for (std::set<char>::const_iterator it = this->_modes.begin(); it != this->_modes.end(); ++it)
        modes += *it;
    
    if (hasMode(MODE_KEY) && !_key.empty())
        modes += " " + this->_key;
    if (hasMode(MODE_LIMIT) &&this->_user_limit > 0)
        modes += " " + itoa(_user_limit);
    
    return modes;
}

// Validations
bool Channel::isPasswordRequired() const
{
    return !this->_password.empty();
}

bool Channel::canUserJoin(std::string const &user, std::string const &password) const
{
    // Check password
    if (isPasswordRequired() && password != this->_password)
        return false;
    
    // Check invite-only mode
    if (hasMode(MODE_INVITE_ONLY) && !isInvited(user))
        return false;
    
    // Check user limit
    if (hasMode(MODE_LIMIT) &&this->_user_limit > 0 &&this->_userlist.size() >= this->_user_limit)
        return false;
    
    // Check key mode
    if (hasMode(MODE_KEY) && !_key.empty() && password != this->_key)
        return false;
    
    return true;
}

bool Channel::canUserSetTopic(std::string const &user) const
{
    if (!hasUser(user))
        return false;
    
    // If topic protection is on, only operators can set topic
    if (hasMode(MODE_TOPIC_PROTECT))
        return isOp(user);
    
    return true;
}

// Connection and Communication
bool Channel::connect(Server *server, int client_fd)
{
    Client *client = server->getClient(client_fd);
    if (!client)
        return false;
    
    std::string nick = client->getNickname();
    
    // Send JOIN confirmation
    std::string join_msg = ":" + nick + "!" + client->getUsername() + "@" + client->getHostname() + 
                          " JOIN #" + this->_name + "\r\n";
    send(client_fd, join_msg.c_str(), join_msg.length(), 0);
    
    // Send topic if exists
    if (!_topic.empty())
    {
        std::string topic_msg = ":" + server->getServerName() + " 332 " + nick + " #" + this->_name + 
                               " :" + this->_topic + "\r\n";
        send(client_fd, topic_msg.c_str(), topic_msg.length(), 0);
    }
    
    // Send NAMES list
    std::string names_msg = ":" + server->getServerName() + " 353 " + nick + " = #" + this->_name + 
                           " :" + getUserListString() + "\r\n";
    send(client_fd, names_msg.c_str(), names_msg.length(), 0);
    
    std::string end_names = ":" + server->getServerName() + " 366 " + nick + " #" + this->_name + 
                           " :End of /NAMES list\r\n";
    send(client_fd, end_names.c_str(), end_names.length(), 0);
    
    client->joinChannel("#" + this->_name);
    return true;
}

void Channel::sendMessage(Server *server, const std::string &sender, std::string const msg, std::string command)
{
    // Parse sender para extrair apenas o nickname
    std::string sender_nick = sender;
    size_t exclamation = sender.find('!');
    if (exclamation != std::string::npos)
        sender_nick = sender.substr(0, exclamation);
    
    std::string formatted_msg = ":" + sender + " " + command + " #" + _name + " " + msg + "\r\n";
    
    for (std::set<std::string>::const_iterator it = _userlist.begin(); it != _userlist.end(); ++it)
    {
        // NÃO enviar para quem mandou a mensagem
        if (*it == sender_nick)
            continue;
            
        Client *client = server->getClientByNick(*it);
        if (client)
        {
            if (send(client->getFd(), formatted_msg.c_str(), formatted_msg.length(), 0) == -1)
                logMessage("ERROR: ", RED, "Failed to send message to " + *it, YELLOW, ERR);
        }
    }
}

// System messages
void Channel::announceJoin(Server *server, Client *client)
{
    std::string join_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + 
                          client->getHostname() + " JOIN #" + this->_name + "\r\n";
    
   this->_boreadcastToChannel(server, join_msg);
}

void Channel::announcePart(Server *server, Client *client, const std::string &reason)
{
    std::string part_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + 
                          client->getHostname() + " PART #" + this->_name;
    if (!reason.empty())
        part_msg += " :" + reason;
    part_msg += "\r\n";
    
   this->_boreadcastToChannel(server, part_msg);
}

void Channel::announceQuit(Server *server, Client *client, const std::string &reason)
{
    std::string quit_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + 
                          client->getHostname() + " QUIT";
    if (!reason.empty())
        quit_msg += " :" + reason;
    quit_msg += "\r\n";
    
   this->_boreadcastToChannel(server, quit_msg, client->getFd());
}

void Channel::announceNickChange(Server *server, const std::string &oldNick, const std::string &newNick)
{
    std::string nick_msg = ":" + oldNick + " NICK :" + newNick + "\r\n";
   this->_boreadcastToChannel(server, nick_msg);
}

// Empty channel checking
bool Channel::isEmpty() const
{
    return this->_userlist.empty();
}

// Private methods
bool Channel::_hasMode(char mode) const
{
    return (this->_modes.find(mode) != this->_modes.end());
}

void Channel::_boreadcastToChannel(Server *server, const std::string &message, int exclude_fd)
{
    for (std::set<std::string>::const_iterator it = this->_userlist.begin(); it != this->_userlist.end(); ++it)
    {
        Client *client = server->getClientByNick(*it);
        if (client && client->getFd() != exclude_fd)
        {
            if (send(client->getFd(), message.c_str(), message.length(), 0) == -1)
                logMessage("ERROR: ", RED, "Failed to broadcast to " + *it, YELLOW, ERR);
        }
    }
}

std::string Channel::_getUserModePrefix(const std::string &nickname) const
{
    if (isOp(nickname))
        return "@";
    return "";
}

bool Channel::updateUserNick(const std::string &oldNick, const std::string &newNick)
{
    if (this->_userlist.find(oldNick) == this->_userlist.end())
        return false; // Usuário não estava no canal
    
    // Remove o nick antigo
    this->_userlist.erase(oldNick);
    
    // Adiciona o novo nick
    this->_userlist.insert(newNick);
    
    // Se era operador, atualiza
    if (this->_operators.find(oldNick) != this->_operators.end())
    {
        this->_operators.erase(oldNick);
        this->_operators.insert(newNick);
    }
    
    // Remove das listas de banidos/convidados se presente
    this->_banned.erase(oldNick);
    this->_invited.erase(oldNick);
    
    logMessage("Nick updated in channel ", GREEN, this->_name + ": " + oldNick + " -> " + newNick, BLUE);
    return true;
}
