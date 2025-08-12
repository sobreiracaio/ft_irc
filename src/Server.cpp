/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/08/12 17:49:04 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(int port, std::string password): _port(port), _password(password), _server_fd(-1)
{
    this->_server_name = "ircserv";
}

Server::~Server()
{
    std::map<int, Client*>::iterator it;
    for (it = this->_clients.begin(); it != this->_clients.end(); it++)
        delete it->second;
    this->_clients.clear();

    std::map<std::string, Channel*>::iterator channel_it;
    for (channel_it = this->_channels.begin(); channel_it != this->_channels.end(); channel_it++)
        delete channel_it->second;
    this->_channels.clear();
    
    if(this->_server_fd > 0)
        close(this->_server_fd);
}

int Server::_createSocket()
{
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_socket == -1)
    {
        logMessage("ERROR: ", RED, "Can't create socket!", YELLOW, ERR);
        return -1;
    }
    
    if (fcntl(listen_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        logMessage("ERROR: ", RED, "Failed to set server socket to non-blocking mode!", YELLOW, ERR);
        close(listen_socket);
        return -1;
    }
    
    this->_server_fd = listen_socket;

    int opt = 1;
    if (setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        logMessage("ERROR: ", RED, "Failed to set socket options!", YELLOW, ERR);
        return (-1);
    }
    
    logMessage("Server socket created successfully!", BLUE, "", RESET);
    return (this->_server_fd);
}

int Server::_bindSocket(void)
{
    memset(&this->_server_addr, 0, sizeof(this->_server_addr));
    this->_server_addr.sin_family = AF_INET;
    this->_server_addr.sin_port = htons(this->_port);
    this->_server_addr.sin_addr.s_addr = INADDR_ANY;
 
    if(bind(this->_server_fd, (struct sockaddr*)&this->_server_addr, sizeof(this->_server_addr)) == -1)
    {
        logMessage("ERROR: ", RED, "Can't bind to IP/port!", YELLOW, ERR);
        return -1;
    }
    logMessage("Bind successful!", BLUE, "", RESET);
    return (0);
}

int Server::_listenSocket()
{
    if(listen(this->_server_fd, SOMAXCONN) == -1)
    {
        logMessage("ERROR: ", RED, "Can't listen!", YELLOW, ERR);
        return -1;
    }
    else
    {
        logMessage("Waiting for connections...", GREEN, "", RESET);
    }
    return (0);
}

bool Server::serverInit()
{
    if(this->_createSocket() == -1 ||
            this->_bindSocket() == -1 ||
            this->_listenSocket() == -1)
        return (false);
    
    // POLL SETUP FOR SERVER SOCKET
    struct pollfd server_pollfd;
    server_pollfd.fd = this->_server_fd;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    this->_poll_fds.push_back(server_pollfd);

    return (true);
}

void Server::run()
{
    while(true)
    {
        int poll_count = poll(&this->_poll_fds[0], this->_poll_fds.size(), -1);

        if (poll_count == -1)
        {
            logMessage("ERROR: ", RED, "Poll failed!", YELLOW, ERR);
            break;
        }
        
        size_t poll_size = this->_poll_fds.size();
        for (size_t i = 0; i < poll_size; i++)
        {
            if (i >= this->_poll_fds.size())
                break;
                
            if(this->_poll_fds[i].revents & POLLIN)
            {
                if(this->_poll_fds[i].fd == this->_server_fd) //NEW CONNECTION
                {
                    this->_acceptNewClient(); //FUNCTION TO ACCEPT CLIENT
                    
                }                                          
                else
                {
                    //HANDLING CLIENT DATA
                    this->_handleClientData(this->_poll_fds[i].fd);
                    
                    if(i >= this->_poll_fds.size())
                        break;
   
                }
            }
            else if(this->_poll_fds[i].revents & (POLLHUP | POLLERR))
            {
                //Client disconnected by socker error
                if(this->_poll_fds[i].fd != this->_server_fd)
                {
                    this->_removeClient(this->_poll_fds[i].fd);
                    poll_size = this->_poll_fds.size(); //Update size;
                    i--; // Adjust index to avoid jumping over elements
                }
            }
        }
    }
}

void Server::_acceptNewClient(void)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(this->_server_fd, (struct sockaddr *)&client_addr, &client_len);
                    
    if (client_socket < 0)
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
            logMessage("ERROR: ", RED, "Accept failed!", YELLOW, ERR);
        return;
    }
    
    if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        logMessage("ERROR: ", RED, "Failed to set client socket non-blocking!", YELLOW, ERR);
        close(client_socket);
        return;
    }
    
    //CREATE NEW CLIENT AND ADD IT TO THE MAP ACCORDING TO ITS FD NUMBER(KEY)

    Client* new_client = new Client(client_socket, client_addr);
    this->_clients[client_socket] = new_client;
    
    //POLL SETUP FOR CLIENT SOCKET
    struct pollfd client_pollfd;
    client_pollfd.fd = client_socket;
    client_pollfd.events = POLLIN;
    client_pollfd.revents = 0;
    this->_poll_fds.push_back(client_pollfd);
}

void Server::_handleClientData(int client_fd)
{
    char buffer[BUFFER_SIZE];
    Client *client = getClient(client_fd);
    
    
    if (!client)
        return;
    
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received <= 0)
    {
        if (bytes_received == 0 || (errno != EWOULDBLOCK && errno != EAGAIN))
        {
            logMessage("Client disconnected! FD = ", RED, itoa(client_fd), YELLOW);
            this->_removeClient(client_fd);
        }
        return;        
    }
    
    buffer[bytes_received] = '\0'; //Make sure the string has finished
    std::string data(buffer);
    logMessage("from FD = " + itoa(client_fd) + ":\n", BLUE, data, WHITE);
    
    //If client is not yet registered
    if(client->getNickname().empty())
    {
        client->setNamesAndPass(data);
        
        //Verifies if pass is matching before continues
        if(!this->_checkPassword(client->getPassword()))
        {
            this->_sendErrorReply(client_fd, ERR_PASSWDMISMATCH, "Password incorrect!");
            this->_removeClient(client_fd);
            return;
        }
        
        std::string nick = this->_checkDoubles(client->getNickname(), client_fd);
        client->setNickname(nick);
        this->_welcomeMessage(client);
        return;
    }
    
    client->appendBuffer(data);
    if(client->isDataComplete())
    {
        std::string complete_data = client->getBuffer();
        int command_code = this->parseCommand(complete_data);
        this->executeCommand(client_fd, command_code, complete_data);
        client->cleanBuffer();
    }
}

void Server::_removeClient(int client_fd)
{
    std::vector<struct pollfd>::iterator poll_it;
    std::map<int, Client*>::iterator client_it;

    
    for (poll_it = this->_poll_fds.begin(); poll_it != this->_poll_fds.end(); poll_it++)
    {
        if(poll_it->fd == client_fd)
        {
            this->_poll_fds.erase(poll_it);
            break;
        }   
    }
    
    client_it = this->_clients.find(client_fd);
    if (client_it != this->_clients.end())
    {
        delete client_it->second;
        this->_clients.erase(client_it);
    }

    close(client_fd);
    logMessage("Client removed! FD = ", RED, itoa(client_fd), YELLOW);
}

bool Server::_checkPassword(std::string const &client_pass)
{
    return (this->_password == client_pass);
}

bool Server::_isValidNickname(const std::string &nickname)
{
    if (nickname.empty() || nickname.length() > MAX_NICK_LENGTH)
        return false;
    
    //First char must be a letter or a valid special char
    char first = nickname[0];
    if (!isalpha(first) && first != '[' && first != ']' && first != '\\' && 
        first != '`' && first != '_' && first != '^' && first != '{' && first != '}')
        return false;
    
    //Other characters can be alphanumericals or special chars
    for (size_t i = 1; i < nickname.length(); i++)
    {
        char c = nickname[i];
        if (!isalnum(c) && c != '[' && c != ']' && c != '\\' && 
            c != '`' && c != '_' && c != '^' && c != '{' && c != '}' && c != '-')
            return false;
    }
    return true;
}

bool Server::_isValidChannelName(const std::string &channelName)
{
    if (channelName.empty() || channelName.length() > MAX_CHANNEL_NAME)
        return false;

    // Channel name cannot contain spaces, commas or control chars
    for (size_t i = 0; i < channelName.length(); i++)
    {
        char c = channelName[i];
        if (c == ' ' || c == ',' || c == '\a' || c == '\0' || c == '\r' || c == '\n')
            return false;
    }
    return true;
}

std::vector<std::string> Server::_splitMessage(const std::string &message)
{
    std::vector<std::string> result;
    std::istringstream iss(message);
    std::string token;
    
    while (iss >> token)
        result.push_back(token);
        
    return result;
}

void Server::_sendNumericReply(int client_fd, int code, const std::string &message)
{
    Client *client = getClient(client_fd);
    if (!client)
        return;
        
    std::ostringstream oss;
    oss << ":" << _server_name << " " << std::setfill('0') << std::setw(3) << code 
        << " " << client->getNickname() << " " << message << "\r\n";
    
    std::string reply = oss.str();
    if (send(client_fd, reply.c_str(), reply.length(), 0) == -1)
        logMessage("ERROR: ", RED, "Failed to send numeric reply!", YELLOW, ERR);
}

void Server::_sendErrorReply(int client_fd, int code, const std::string &message)
{
    Client *client = getClient(client_fd);
    if (!client)
        return;
        
    std::ostringstream oss;
    oss << ":" << _server_name << " " << std::setfill('0') << std::setw(3) << code 
        << " " << (client->getNickname().empty() ? "*" : client->getNickname()) 
        << " :" << message << "\r\n";
    
    std::string reply = oss.str();
    if (send(client_fd, reply.c_str(), reply.length(), 0) == -1)
        logMessage("ERROR: ", RED, "Failed to send error reply!", YELLOW, ERR);
}

Client *Server::getClient(int client_fd)
{
    std::map<int, Client*>::iterator it = this->_clients.find(client_fd); 
    
    if (it != this->_clients.end())
        return it->second;
    else
        return (NULL);
}

int Server::getServerFd(void)
{
    return this->_server_fd;
}

std::string Server::getServerName(void) const
{
    return (this->_server_name);
}

int Server::parseCommand(const std::string& data)
{
    std::vector<std::string> tokens = _splitMessage(data);
    if (tokens.empty())
        return NO_COMM;
    
    std::string command = tokens[0];
    
    if(command == "JOIN")
        return JOIN;
    if(command == "PRIVMSG")
        return PRIVMSG;
    if(command == "NICK")
        return NICK;
    if(command == "QUIT")
        return QUIT;
    if(command == "PART")
        return PART;
    if(command == "KICK")
        return KICK;
    if(command == "INVITE")
        return INVITE;
    if(command == "TOPIC")
        return TOPIC;
    if(command == "MODE")
        return MODE;
        
    return NO_COMM;
}

void Server::executeCommand(int client_fd, int command_code, std::string const &data)
{
    switch (command_code)
    {
    case JOIN:
        this->joinChannel(data, client_fd);
        break;
    case PRIVMSG:
        this->privateMsg(data, client_fd);
        break;
    case NICK:
        this->changeNick(data, client_fd);
        break;
    case QUIT:
        this->quitServer(data, client_fd);
        break;
    case PART:
        this->partChannel(data, client_fd);
        break;
    case KICK:
        this->kickUser(data, client_fd);
        break;
    case INVITE:
        this->inviteUser(data, client_fd);
        break;
    case TOPIC:
        this->topicCommand(data, client_fd);
        break;
    case MODE:
        this->modeCommand(data, client_fd);
        break;
    default:
        this->_sendErrorReply(client_fd, ERR_UNKNOWNCOMMAND, "Unknown command");
        break;
    }
}

std::string Server::_checkDoubles(std::string const &nickname, int client_fd)
{
    std::string modifiedNickname = nickname;
    size_t pos = modifiedNickname.find('\r');

    if(pos != std::string::npos)
        modifiedNickname.erase(pos);
    
    pos = modifiedNickname.find('\n');
    if(pos != std::string::npos)
        modifiedNickname.erase(pos);
    
    if (!_isValidNickname(modifiedNickname))
    {
        this->_sendErrorReply(client_fd, ERR_ERRONEUSNICKNAME, "Erroneous nickname");
        return modifiedNickname;
    }
    
    std::map<int, Client*>::iterator it;
    for (it = this->_clients.begin(); it != this->_clients.end(); it++)
    {
        Client *client = it->second;
        if((client->getNickname() == modifiedNickname) && client->getFd() != client_fd)
        {
            this->_sendErrorReply(client_fd, ERR_NICKNAMEINUSE, modifiedNickname + " :Nickname is already in use");
            modifiedNickname += "_";
            break;
        }
    }
    return modifiedNickname;
}

void Server::changeNick(std::string const &data, int client_fd)
{
    std::vector<std::string> tokens = _splitMessage(data);
    
    if (tokens.size() < 2)
    {
        this->_sendErrorReply(client_fd, ERR_NONICKNAMEGIVEN, "No nickname given");
        return;
    }
    
    Client *client = this->_clients[client_fd];
    if (!client)
        return;
    
    std::string new_nickname = this->_checkDoubles(tokens[1], client_fd);
    std::string old_nick = client->getNickname();
    
    std::string msg = ":" + old_nick + "!" + client->getUsername() + "@" + client->getHostname() + 
                      " NICK :" + new_nickname + "\r\n";
    
    if (send(client_fd, msg.c_str(), msg.length(), 0) == -1)
        logMessage("ERROR: ", RED, "Failed to send NICK response!", YELLOW, ERR);
        
    client->setNickname(new_nickname);
}

void Server::quitServer(std::string const &data, int client_fd)
{
    Client *client = getClient(client_fd);
    if (!client)
        return;
    
    std::string quit_msg = "Client quit";
    size_t colon_pos = data.find(':');
    if (colon_pos != std::string::npos)
        quit_msg = data.substr(colon_pos + 1);
    
    // Notificar outros clientes nos mesmos canais
    // (implementação simplificada - deveria notificar apenas canais em comum)
    
    logMessage("Client quit! Nick: ", YELLOW, client->getNickname(), GREEN);
    this->_removeClient(client_fd);
}

void Server::kickUser(std::string const &data, int client_fd)
{
    std::vector<std::string> tokens = _splitMessage(data);
    
    if (tokens.size() < 3)
    {
        this->_sendErrorReply(client_fd, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
        return;
    }
    
    std::string channelName = tokens[1];
    std::string targetNick = tokens[2];
    std::string reason = "Kicked";
    
    // Extract reason if provided
    size_t colon_pos = data.find(':', 5);
    if (colon_pos != std::string::npos)
        reason = data.substr(colon_pos + 1);
    
    if (channelName[0] == '#')
        channelName = channelName.substr(1);
    
    Channel *channel = getChannelByName(channelName);
    if (!channel)
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :No such channel");
        return;
    }
    
    Client *kicker = getClient(client_fd);
    if (!kicker)
        return;
    
    // Check if kicker is in channel and is operator
    if (!channel->hasUser(kicker->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_NOTONCHANNEL, "#" + channelName + " :You're not on that channel");
        return;
    }
    
    if (!channel->isOp(kicker->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_CHANOPRIVSNEEDED, "#" + channelName + " :You're not channel operator");
        return;
    }
    
    // Check if target exists and is in channel
    Client *target = getClientByNick(targetNick);
    if (!target)
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHNICK, targetNick + " :No such nick/channel");
        return;
    }
    
    if (!channel->hasUser(targetNick))
    {
        this->_sendErrorReply(client_fd, ERR_USERNOTINCHANNEL, targetNick + " #" + channelName + " :They aren't on that channel");
        return;
    }
    
    // Send KICK message to all channel members
    std::string kick_msg = ":" + kicker->getNickname() + "!" + kicker->getUsername() + "@" + 
                          kicker->getHostname() + " KICK #" + channelName + " " + targetNick + " :" + reason + "\r\n";
    
    // Broadcast to channel
    std::vector<std::string> users = channel->getUserList();
    for (size_t i = 0; i < users.size(); i++)
    {
        Client *user = getClientByNick(users[i]);
        if (user)
        {
            if (send(user->getFd(), kick_msg.c_str(), kick_msg.length(), 0) == -1)
                logMessage("ERROR: ", RED, "Failed to send KICK message", YELLOW, ERR);
        }
    }
    
    // Remove user from channel
    channel->removeUser(targetNick);
    target->leaveChannel("#" + channelName);
    
    logMessage("User kicked from channel ", YELLOW, channelName + ": " + targetNick, RED);
}

void Server::inviteUser(std::string const &data, int client_fd)
{
    std::vector<std::string> tokens = _splitMessage(data);
    
    if (tokens.size() < 3)
    {
        this->_sendErrorReply(client_fd, ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters");
        return;
    }
    
    std::string targetNick = tokens[1];
    std::string channelName = tokens[2];
    
    if (channelName[0] == '#')
        channelName = channelName.substr(1);
    
    Client *inviter = getClient(client_fd);
    if (!inviter)
        return;
    
    Channel *channel = getChannelByName(channelName);
    if (!channel)
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :No such channel");
        return;
    }
    
    // Check if inviter is in channel
    if (!channel->hasUser(inviter->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_NOTONCHANNEL, "#" + channelName + " :You're not on that channel");
        return;
    }
    
    // Check if inviter has permission (if channel is +i, only ops can invite)
    if (channel->hasMode(MODE_INVITE_ONLY) && !channel->isOp(inviter->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_CHANOPRIVSNEEDED, "#" + channelName + " :You're not channel operator");
        return;
    }
    
    Client *target = getClientByNick(targetNick);
    if (!target)
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHNICK, targetNick + " :No such nick/channel");
        return;
    }
    
    // Check if target is already in channel
    if (channel->hasUser(targetNick))
    {
        this->_sendErrorReply(client_fd, ERR_USERONCHANNEL, targetNick + " #" + channelName + " :is already on channel");
        return;
    }
    
    // Add to invite list
    channel->inviteUser(targetNick);
    
    // Send invite to target
    std::string invite_msg = ":" + inviter->getNickname() + "!" + inviter->getUsername() + "@" + 
                            inviter->getHostname() + " INVITE " + targetNick + " #" + channelName + "\r\n";
    
    if (send(target->getFd(), invite_msg.c_str(), invite_msg.length(), 0) == -1)
        logMessage("ERROR: ", RED, "Failed to send INVITE", YELLOW, ERR);
    
    // Confirm to inviter
    std::string confirm_msg = ":" + _server_name + " 341 " + inviter->getNickname() + " " + 
                             targetNick + " #" + channelName + "\r\n";
    
    if (send(client_fd, confirm_msg.c_str(), confirm_msg.length(), 0) == -1)
        logMessage("ERROR: ", RED, "Failed to send INVITE confirmation", YELLOW, ERR);
    
    logMessage("User invited to channel ", GREEN, channelName + ": " + targetNick, BLUE);
}

void Server::privateMsg(std::string const &data, int client_fd)
{
    Client *sender = getClient(client_fd);
    if (!sender)
        return;
    
    std::vector<std::string> tokens = _splitMessage(data);
    
    if (tokens.size() < 2)
    {
        this->_sendErrorReply(client_fd, ERR_NORECIPIENT, "No recipient given (PRIVMSG)");
        return;
    }
    
    size_t colon = data.find(':', 8);
    if (colon == std::string::npos)
    {
        this->_sendErrorReply(client_fd, ERR_NOTEXTTOSEND, "No text to send");
        return;
    }
    
    std::string target = tokens[1];
    std::string message = data.substr(colon);
    
    if (target[0] == '#')
    {
        // Message to channel
        std::string channel_name = target.substr(1);
        Channel *channel = getChannelByName(channel_name);
        
        if (!channel)
        {
            this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, target + " :No such channel");
            return;
        }
        
        // Check if user can send to channel
        if (channel->hasMode(MODE_NO_EXTERNAL_MSGS) && !channel->hasUser(sender->getNickname()))
        {
            this->_sendErrorReply(client_fd, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
            return;
        }
        
        if (channel->hasMode(MODE_MODERATED) && !channel->isOp(sender->getNickname()))
        {
            this->_sendErrorReply(client_fd, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
            return;
        }
        
        // Send message to channel
        std::string sender_prefix = sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname();
        channel->sendMessage(this, sender_prefix, message);
    }
    else
    {
        // Private message
        Client *receiver = getClientByNick(target);
        if (!receiver)
        {
            this->_sendErrorReply(client_fd, ERR_NOSUCHNICK, target + " :No such nick/channel");
            return;
        }
        
        std::string returnMsg = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" +
                                sender->getHostname() + " PRIVMSG " + target + " " + message + "\r\n";
        
        if (send(receiver->getFd(), returnMsg.c_str(), returnMsg.length(), 0) == -1)
            logMessage("ERROR: ", RED, "Failed to send private message!", YELLOW, ERR);
    }
}

Client *Server::getClientByNick(std::string const &nick)
{
    std::string formattedNick = nick;
    
    // Remove espaços e quebras de linha
    size_t pos = formattedNick.find_first_of(" \r\n");
    if (pos != std::string::npos)
        formattedNick.erase(pos);

    std::map<int, Client*>::iterator client_it;
    for (client_it = this->_clients.begin(); client_it != this->_clients.end(); client_it++)
    {
        Client *temp = client_it->second;
        if(temp->getNickname() == formattedNick)
            return temp;
    }
    return NULL;
}

void Server::joinChannel(std::string const &data, int client_fd)
{
    std::vector<std::string> tokens = _splitMessage(data);
    
    if (tokens.size() < 2)
    {
        this->_sendErrorReply(client_fd, ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
        return;
    }
    
    std::string channelName = tokens[1];
    std::string channelPassword = (tokens.size() > 2) ? tokens[2] : "";
    
    if (channelName[0] != '#')
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
        return;
    }
    
    channelName = channelName.substr(1); // Remove o #
    
    if (!_isValidChannelName(channelName))
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :No such channel");
        return;
    }
    
    Channel *channel = this->getChannelByName(channelName);
    
    if (!channel)
    {
        // Criar novo canal
        Channel *new_channel = new Channel(channelName, channelPassword);
        this->_channels[channelName] = new_channel;
        new_channel->addUser(this->getClient(client_fd)->getNickname());
        new_channel->connect(this, client_fd);
    }
    else
    {
        // Entrar em canal existente
        channel->addUser(this->getClient(client_fd)->getNickname());
        channel->connect(this, client_fd);
    }
}

void Server::partChannel(std::string const &data, int client_fd)
{
    std::vector<std::string> tokens = _splitMessage(data);
    
    if (tokens.size() < 2)
    {
        this->_sendErrorReply(client_fd, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
        return;
    }
    
    std::string channelName = tokens[1];
    if (channelName[0] == '#')
        channelName = channelName.substr(1);
    
    Channel *channel = getChannelByName(channelName);
    if (!channel)
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :No such channel");
        return;
    }
    
    Client *client = getClient(client_fd);
    if (!client)
        return;
    
    if (!channel->hasUser(client->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_NOTONCHANNEL, "#" + channelName + " :You're not on that channel");
        return;
    }
    
    // Get part reason if provided
    std::string reason = "";
    size_t colon_pos = data.find(':', 5);
    if (colon_pos != std::string::npos)
        reason = data.substr(colon_pos + 1);
    
    // Announce part to channel
    channel->announcePart(this, client, reason);
    
    // Remove user from channel
    channel->removeUser(client->getNickname());
    client->leaveChannel("#" + channelName);
    
    // Remove channel if empty
    if (channel->isEmpty())
    {
        delete channel;
        _channels.erase(channelName);
        logMessage("Empty channel removed: ", YELLOW, channelName, RED);
    }
    
    logMessage("User left channel ", YELLOW, channelName + ": " + client->getNickname(), BLUE);
}

void Server::topicCommand(std::string const &data, int client_fd)
{
    std::vector<std::string> tokens = _splitMessage(data);
    
    if (tokens.size() < 2)
    {
        this->_sendErrorReply(client_fd, ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");
        return;
    }
    
    std::string channelName = tokens[1];
    if (channelName[0] == '#')
        channelName = channelName.substr(1);
    
    Channel *channel = getChannelByName(channelName);
    if (!channel)
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :No such channel");
        return;
    }
    
    Client *client = getClient(client_fd);
    if (!client)
        return;
    
    // Check if user is in channel
    if (!channel->hasUser(client->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_NOTONCHANNEL, "#" + channelName + " :You're not on that channel");
        return;
    }
    
    size_t colon_pos = data.find(':', 6);
    
    if (colon_pos == std::string::npos) // No new topic, just viewing
    {
        std::string topic = channel->getTopic();
        if (topic.empty())
        {
            std::string no_topic = ":" + _server_name + " 331 " + client->getNickname() + 
                                  " #" + channelName + " :No topic is set\r\n";
            send(client_fd, no_topic.c_str(), no_topic.length(), 0);
        }
        else
        {
            std::string topic_msg = ":" + _server_name + " 332 " + client->getNickname() + 
                                   " #" + channelName + " :" + topic + "\r\n";
            send(client_fd, topic_msg.c_str(), topic_msg.length(), 0);
        }
        return;
    }
    
    // Setting new topic
    if (!channel->canUserSetTopic(client->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_CHANOPRIVSNEEDED, "#" + channelName + " :You're not channel operator");
        return;
    }
    
    std::string newTopic = data.substr(colon_pos + 1);
    channel->setTopic(newTopic, client->getNickname());
    
    // Broadcast topic change to channel
    std::string topic_change = ":" + client->getNickname() + "!" + client->getUsername() + "@" + 
                              client->getHostname() + " TOPIC #" + channelName + " :" + newTopic + "\r\n";
    
    std::vector<std::string> users = channel->getUserList();
    for (size_t i = 0; i < users.size(); i++)
    {
        Client *user = getClientByNick(users[i]);
        if (user)
        {
            if (send(user->getFd(), topic_change.c_str(), topic_change.length(), 0) == -1)
                logMessage("ERROR: ", RED, "Failed to send TOPIC change", YELLOW, ERR);
        }
    }
    
    logMessage("Topic changed for channel ", BLUE, channelName + ": " + newTopic, GREEN);
}


Channel *Server::getChannelByName(std::string const &name)
{
    std::map<std::string, Channel*>::iterator channel_it;

    for (channel_it = this->_channels.begin(); channel_it != this->_channels.end(); channel_it++)
    {
        Channel *temp = channel_it->second;
    
        if(temp->getName() == name)
            return (temp);
    }
     return (NULL);
}

void Server::modeCommand(std::string const &data, int client_fd)
{
    std::vector<std::string> tokens = _splitMessage(data);
    
    if (tokens.size() < 2)
    {
        this->_sendErrorReply(client_fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
        return;
    }
    
    std::string target = tokens[1];
    
    // Channel mode
    if (target[0] == '#')
    {
        handleChannelMode(data, client_fd);
    }
    else
    {
        // User mode (não implementado completamente)
        this->_sendErrorReply(client_fd, ERR_UNKNOWNMODE, "User modes not implemented");
    }
}

void Server::handleChannelMode(std::string const &data, int client_fd)
{
    std::vector<std::string> tokens = _splitMessage(data);
    
    std::string channelName = tokens[1];
    if (channelName[0] == '#')
        channelName = channelName.substr(1);
    
    Channel *channel = getChannelByName(channelName);
    if (!channel)
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :No such channel");
        return;
    }
    
    Client *client = getClient(client_fd);
    if (!client)
        return;
    
    // Check if user is in channel
    if (!channel->hasUser(client->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_NOTONCHANNEL, "#" + channelName + " :You're not on that channel");
        return;
    }
    
    if (tokens.size() < 3) // Just viewing modes
    {
        std::string mode_msg = ":" + _server_name + " 324 " + client->getNickname() + 
                              " #" + channelName + " " + channel->getModeString() + "\r\n";
        send(client_fd, mode_msg.c_str(), mode_msg.length(), 0);
        return;
    }
    
    // Check operator privileges for mode changes
    if (!channel->isOp(client->getNickname()))
    {
        this->_sendErrorReply(client_fd, ERR_CHANOPRIVSNEEDED, "#" + channelName + " :You're not channel operator");
        return;
    }
    
    std::string modestring = tokens[2];
    size_t param_index = 3;
    bool adding = true;
    
    std::string changes;
    std::vector<std::string> change_params;
    
    for (size_t i = 0; i < modestring.length(); i++)
    {
        char c = modestring[i];
        
        if (c == '+')
        {
            adding = true;
            if (changes.empty() || changes[changes.length()-1] != '+')
                changes += "+";
        }
        else if (c == '-')
        {
            adding = false;
            if (changes.empty() || changes[changes.length()-1] != '-')
                changes += "-";
        }
        else
        {
            std::string param;
            
            // Get parameter if needed
            if ((c == 'k' || c == 'l' || c == 'o') && adding && param_index < tokens.size())
            {
                param = tokens[param_index++];
            }
            else if (c == 'o' && !adding && param_index < tokens.size())
            {
                param = tokens[param_index++];
            }
            
            // Apply mode change
            bool success = false;
            
            switch (c)
            {
                case 'i': // invite-only
                case 't': // topic protection  
                case 'n': // no external messages
                case 's': // secret
                case 'p': // private
                case 'm': // moderated
                    success = channel->setMode(c, adding);
                    break;
                    
                case 'k': // key (password)
                    if (adding && !param.empty())
                        success = channel->setMode(c, adding, param);
                    else if (!adding)
                        success = channel->setMode(c, adding);
                    break;
                    
                case 'l': // user limit
                    if (adding && !param.empty() && isNum(param))
                        success = channel->setMode(c, adding, param);
                    else if (!adding)
                        success = channel->setMode(c, adding);
                    break;
                    
                case 'o': // operator privilege
                    if (!param.empty())
                    {
                        Client *target = getClientByNick(param);
                        if (target && channel->hasUser(param))
                        {
                            if (adding)
                                success = channel->addOp(param);
                            else
                                success = channel->removeOp(param);
                        }
                    }
                    break;
                    
                default:
                    this->_sendErrorReply(client_fd, ERR_UNKNOWNMODE, std::string(1, c) + " :is unknown mode char to me");
                    continue;
            }
            
            if (success)
            {
                changes += c;
                if (!param.empty())
                    change_params.push_back(param);
            }
        }
    }
    
    // Broadcast mode changes
    if (!changes.empty())
    {
        std::string mode_change = ":" + client->getNickname() + "!" + client->getUsername() + "@" + 
                                 client->getHostname() + " MODE #" + channelName + " " + changes;
        
        for (size_t i = 0; i < change_params.size(); i++)
            mode_change += " " + change_params[i];
        
        mode_change += "\r\n";
        
        std::vector<std::string> users = channel->getUserList();
        for (size_t i = 0; i < users.size(); i++)
        {
            Client *user = getClientByNick(users[i]);
            if (user)
            {
                if (send(user->getFd(), mode_change.c_str(), mode_change.length(), 0) == -1)
                    logMessage("ERROR: ", RED, "Failed to send MODE change", YELLOW, ERR);
            }
        }
        
        logMessage("Mode changed for channel ", BLUE, channelName + ": " + changes, GREEN);
    }
}


void Server::_welcomeMessage(Client* client)
{
    
    std::stringstream welcome;
    welcome << "##############################\n";
    welcome << "#   Bem-vindo ao IRC Server  #\n";
    welcome << "#                            #\n";
    welcome << "#  Comandos disponíveis:     #\n";
    welcome << "#                            #\n";
    welcome << "#  /JOIN #canal [senha]      #\n";
    welcome << "#  /PRIVMSG alvo :mensagem   #\n";
    welcome << "#  /NICK novonick            #\n";
    welcome << "#  /QUIT                     #\n";
    welcome << "##############################\n";
    
    std::string line;

    while(getline(welcome, line, '\n'))
    {
        std::string msg = ":" + client->getHostname() + " 001 " + client->getNickname() + " :" + line + "\r\n";
        send(client->getFd(), msg.c_str(), msg.length(), 0);
    }
}
