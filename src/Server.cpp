/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/08/12 14:34:54 by caio             ###   ########.fr       */
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
    
    size_t colon = data.find(':', 8); // Procura após "PRIVMSG "
    if (colon == std::string::npos)
    {
        this->_sendErrorReply(client_fd, ERR_NOTEXTTOSEND, "No text to send");
        return;
    }
    
    std::string target = tokens[1];
    std::string message = data.substr(colon);
    
    if (target[0] == '#')
    {
        // Mensagem para canal
        std::string channel_name = target.substr(1);
        Channel *channel = this->getChannelByName(channel_name);
        
        if (!channel)
        {
            this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, target + " :No such channel");
            return;
        }
        
        // Implementar envio para canal (precisará de melhorias na classe Channel)
    }
    else
    {
        // Mensagem privada
        Client *receiver = this->getClientByNick(target);
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
    
    Channel *channel = this->getChannelByName(channelName);
    if (!channel)
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :No such channel");
        return;
    }
    
    Client *client = getClient(client_fd);
    if (!client)
        return;
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
