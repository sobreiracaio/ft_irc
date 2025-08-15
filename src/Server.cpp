/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/08/15 14:13:35 by caio             ###   ########.fr       */
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

// Substitua a função _handleClientData no Server.cpp
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
    
    buffer[bytes_received] = '\0';
    
    // Limpa caracteres de controle problemáticos do buffer
    std::string data;
    for (int i = 0; i < bytes_received; i++)
    {
        unsigned char c = static_cast<unsigned char>(buffer[i]);
        
        // Filtrar caracteres de controle perigosos, mas manter \r\n
        if (c >= 32 || c == '\r' || c == '\n' || c == '\t')
        {
            data += c;
        }
        else if (c == 4) // Ctrl+D (EOF)
        {
            // Ignora Ctrl+D mas continua processamento
            logMessage("Ctrl+D received from FD ", YELLOW, itoa(client_fd), WHITE);
            continue;
        }
        // Outros caracteres de controle são ignorados
    }
    
    if (data.empty())
        return;
    
    logMessage("from FD = " + itoa(client_fd) + ":\n", BLUE, data, WHITE);
    
    // SEMPRE adicionar dados ao buffer primeiro
    client->appendBuffer(data);
    
    // Se cliente não está registrado, processa registro
    if(!client->isRegistered())
    {
        bool processed_any_command = false;
        
        // Processa todos os comandos de registro disponíveis no buffer
        while(client->isDataComplete())
        {
            std::string message = client->getNextCompleteMessage();
            
            if (message.empty())
                break;
            
            logMessage("Processing registration command: ", CYAN, message, WHITE);
            processed_any_command = true;
            
            // Processa comandos de registro
            if (message.length() >= 5)
            {
                std::string cmd = message.substr(0, 5);
                
                if(cmd == "PASS ")
                {
                    client->parsePassCommand(message);
                }
                else if(cmd == "NICK ")
                {
                    client->parseNickCommand(message);
                }
                else if(cmd == "USER ")
                {
                    client->parseUserCommand(message);
                }
            }
            // CORREÇÃO: Para nc, também aceita comandos sem espaço extra
            else if (message.length() >= 4)
            {
                std::string cmd = message.substr(0, 4);
                if(cmd == "NICK")
                {
                    // Processa NICK mesmo sem espaço (para nc)
                    if(message.length() > 4)
                        client->parseNickCommand("NICK " + message.substr(4));
                }
                else if(cmd == "PASS")
                {
                    if(message.length() > 4)
                        client->parsePassCommand("PASS " + message.substr(4));
                }
                else if(cmd == "USER")
                {
                    if(message.length() > 4)
                        client->parseUserCommand("USER " + message.substr(4));
                }
            }
        }
        
        // NOVO: Se processamos comandos e temos PASS + NICK, tenta completar registro
        if(processed_any_command && !client->getPassword().empty() && !client->getNickname().empty())
        {
            // Força verificação de registro (que auto-gerará USER se necessário)
            client->checkRegistrationComplete();
        }
        
        // Só verifica se está totalmente registrado após processar todos os comandos
        if (client->isRegistered())
        {
            // Verifica se a senha está correta APENAS quando totalmente registrado
            if(!this->_checkPassword(client->getPassword()))
            {
                logMessage("Password check failed for client: ", RED, client->getNickname(), YELLOW);
                this->_sendErrorReply(client_fd, ERR_PASSWDMISMATCH, "Password incorrect!");
                // Aguarda um pouco antes de desconectar para evitar "connection reset"
                usleep(100000); // 100ms
                this->_removeClient(client_fd);
                return;
            }
            
            // Verifica duplicatas de nickname
            std::string nick = this->_checkDoubles(client->getNickname(), client_fd);
            client->setNickname(nick);
            this->_welcomeMessage(client);
            logMessage("Client fully registered: ", GREEN, client->getNickname(), BLUE);
        }
        return;
    }
    
    // Cliente já registrado - processa comandos normais
    while(client->isDataComplete())
    {
        std::string message = client->getNextCompleteMessage();
        
        if (message.empty())
            break;
            
        int command_code = this->parseCommand(message);
        
        // Execute command and check if the client was removed
        bool client_still_exists = this->executeCommand(client_fd, command_code, message);
        
        if (!client_still_exists)
        {
            return; // Cliente foi removido, não tenta acessar mais
        }
        
        // Verifica se o cliente ainda existe após execução do comando
        client = getClient(client_fd);
        if (!client)
            return; // Cliente foi removido, sair imediatamente
    }
}

void Server::_removeClient(int client_fd)
{
    // Remove from poll fds first
    std::vector<struct pollfd>::iterator poll_it;
    for (poll_it = this->_poll_fds.begin(); poll_it != this->_poll_fds.end(); poll_it++)
    {
        if(poll_it->fd == client_fd)
        {
            this->_poll_fds.erase(poll_it);
            break;
        }   
    }
    
    // Find and remove client
    std::map<int, Client*>::iterator client_it = this->_clients.find(client_fd);
    if (client_it != this->_clients.end())
    {
        Client *client = client_it->second;
        
        // Remove client from all channels before deleting
        if (client)
        {
            std::set<std::string> client_channels = client->getChannels();
            std::string client_nick = client->getNickname();
            
            for (std::set<std::string>::const_iterator it = client_channels.begin(); 
                 it != client_channels.end(); ++it)
            {
                std::string channel_name = *it;
                if (channel_name[0] == '#')
                    channel_name = channel_name.substr(1);
                    
                Channel *channel = getChannelByName(channel_name);
                if (channel)
                {
                    channel->removeUser(client_nick);
                    
                    // Remove empty channels
                    if (channel->isEmpty())
                    {
                        delete channel;
                        _channels.erase(channel_name);
                        logMessage("Empty channel removed: ", YELLOW, channel_name, RED);
                    }
                }
            }
        }
        
        // Now safe to delete client
        delete client;
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
    
    // Verificação mais robusta: existe no mapa E o ponteiro não é nulo
    if (it != this->_clients.end() && it->second != NULL)
        return it->second;
    else
        return NULL;
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

bool Server::executeCommand(int client_fd, int command_code, std::string const &data)
{
    switch (command_code)
    {
    case JOIN:
        this->joinChannel(data, client_fd);
        return true; 
        
    case PRIVMSG:
        this->privateMsg(data, client_fd);
        return true;
        
    case NICK:
        this->changeNick(data, client_fd);
        return true;
        
    case QUIT:
        this->quitServer(data, client_fd);
        return false; //client removed
        
    case PART:
        this->partChannel(data, client_fd);
        return true;
        
    case KICK:
        this->kickUser(data, client_fd);
        return true;
        
    case INVITE:
        this->inviteUser(data, client_fd);
        return true;
        
    case TOPIC:
        this->topicCommand(data, client_fd);
        return true;
        
    case MODE:
        this->modeCommand(data, client_fd);
        return true;
        
    default:
        //this->_sendErrorReply(client_fd, ERR_UNKNOWNCOMMAND, "Unknown command");
        return true;
    }
}

std::string Server::_checkDoubles(std::string const &nickname, int client_fd)
{
    std::string modifiedNickname = nickname;
    
    // Remove caracteres de controle mais agressivamente
    std::string cleanNick;
    for (size_t i = 0; i < modifiedNickname.length(); i++)
    {
        unsigned char c = static_cast<unsigned char>(modifiedNickname[i]);
        // Remove todos os caracteres de controle exceto espaços normais
        if (c >= 32 && c != 127) // Caracteres imprimíveis
            cleanNick += c;
        else if (c == ' ') // Permitir espaço, mas será tratado depois
            break; // Para no primeiro espaço
    }
    modifiedNickname = cleanNick;
    
    // Remove espaços extras
    size_t space_pos = modifiedNickname.find(' ');
    if (space_pos != std::string::npos)
        modifiedNickname.erase(space_pos);
    
    if (!_isValidNickname(modifiedNickname))
    {
        this->_sendErrorReply(client_fd, ERR_ERRONEUSNICKNAME, "Erroneous nickname");
        return modifiedNickname;
    }
    
    // Verificação melhorada para nicks duplicados
    std::map<int, Client*>::iterator it;
    for (it = this->_clients.begin(); it != this->_clients.end(); it++)
    {
        Client *client = it->second;
        // Verifica se o cliente existe, está ativo e tem o mesmo nick
        if (client && client->getFd() != client_fd && !client->getNickname().empty() && 
            client->getNickname() == modifiedNickname)
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
    
    std::string old_nick = client->getNickname();
    std::string new_nickname = this->_checkDoubles(tokens[1], client_fd);
    
    // Se o nick não mudou (incluindo casos onde foi adicionado _), ainda processa
    if (old_nick == new_nickname && tokens[1] == new_nickname)
        return; // Mesmo nick, sem mudança necessária
    
    // Prepara a mensagem NICK
    std::string nick_msg = ":" + old_nick + "!" + client->getUsername() + "@" + 
                          client->getHostname() + " NICK :" + new_nickname + "\r\n";
    
    // Coleta todos os clientes que precisam ser notificados
    std::set<int> clients_to_notify;
    std::set<std::string> client_channels = client->getChannels();
    
    // Para cada canal onde o usuário está
    for (std::set<std::string>::const_iterator it = client_channels.begin(); 
         it != client_channels.end(); ++it)
    {
        std::string channel_name = *it;
        if (channel_name[0] == '#')
            channel_name = channel_name.substr(1);
        
        Channel *channel = getChannelByName(channel_name);
        if (channel)
        {
            // Atualiza o nick no canal usando o novo método
            channel->updateUserNick(old_nick, new_nickname);
            
            // Coleta usuários do canal para notificar
            std::vector<std::string> users = channel->getUserList();
            for (size_t i = 0; i < users.size(); i++)
            {
                Client *user = getClientByNick(users[i]);
                if (user && user->getFd() != client_fd)
                {
                    clients_to_notify.insert(user->getFd());
                }
            }
        }
    }
    
    // Atualiza o nickname do cliente
    client->setNickname(new_nickname);
    
    // Envia confirmação para o próprio cliente
    if (send(client_fd, nick_msg.c_str(), nick_msg.length(), 0) == -1)
        logMessage("ERROR: ", RED, "Failed to send NICK response!", YELLOW, ERR);
    
    // Notifica todos os clientes coletados
    for (std::set<int>::iterator it = clients_to_notify.begin(); 
         it != clients_to_notify.end(); ++it)
    {
        if (send(*it, nick_msg.c_str(), nick_msg.length(), 0) == -1)
            logMessage("ERROR: ", RED, "Failed to broadcast NICK change", YELLOW, ERR);
    }
    
    logMessage("Nickname changed: ", GREEN, old_nick + " -> " + new_nickname, BLUE);
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
    
    // Gets channel list before removing client
    std::set<std::string> client_channels = client->getChannels();
    std::string client_nick = client->getNickname();
    std::string client_user = client->getUsername();
    std::string client_host = client->getHostname();
    
    // Notify channels about the quitting
    for (std::set<std::string>::const_iterator it = client_channels.begin(); 
         it != client_channels.end(); ++it)
    {
        std::string channel_name = *it;
        if (channel_name[0] == '#')
            channel_name = channel_name.substr(1);
            
        Channel *channel = getChannelByName(channel_name);
        if (channel)
        {
            // Remove user from channel
            channel->removeUser(client_nick);
            
            // Anuncia QUIT para outros usuários do canal
            channel->announceQuit(this, client, quit_msg);
            
            // Remove channel if it is empty
            if (channel->isEmpty())
            {
                delete channel;
                _channels.erase(channel_name);
                logMessage("Empty channel removed: ", YELLOW, channel_name, RED);
            }
        }
    }
    
    logMessage("Client quit! Nick: ", YELLOW, client_nick, GREEN);
    
    // Removes client
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
        
        //Verify is user is in channel (for +n mode)
        if (channel->hasMode(MODE_NO_EXTERNAL_MSGS) && !channel->hasUser(sender->getNickname()))
        {
            this->_sendErrorReply(client_fd, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
            return;
        }
        
        // Verify moderated mode
        if (channel->hasMode(MODE_MODERATED) && !channel->isOp(sender->getNickname()))
        {
            this->_sendErrorReply(client_fd, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
            return;
        }
        
        // Send message to channel, excluding sender (to avoid double message for the sender)
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
    
    // Remove spaces and other chars
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
    
    channelName = channelName.substr(1); // Remove the #
    
    if (!_isValidChannelName(channelName))
    {
        this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :No such channel");
        return;
    }
    
    Client *client = getClient(client_fd);
    if (!client)
        return;
    
    // Verify if the client is already on channel
    if (client->isInChannel("#" + channelName))
        return;
    
    Channel *channel = this->getChannelByName(channelName);
    bool is_new_channel = false;
    
    if (!channel)
    {
        // Create new channel
        channel = new Channel(channelName, channelPassword);
        this->_channels[channelName] = channel;
        is_new_channel = true;
    }
    
    // Verify if client can join channel
    if (!channel->canUserJoin(client->getNickname(), channelPassword))
    {
        // Send error based on reason of not joining
        if (channel->isBanned(client->getNickname()))
            this->_sendErrorReply(client_fd, ERR_BANNEDFROMCHAN, "#" + channelName + " :Cannot join channel (+b)");
        else if (channel->hasMode(MODE_INVITE_ONLY) && !channel->isInvited(client->getNickname()))
            this->_sendErrorReply(client_fd, ERR_INVITEONLYCHAN, "#" + channelName + " :Cannot join channel (+i)");
        else if (channel->hasMode(MODE_LIMIT) && channel->getUserLimit() > 0 && channel->getUserCount() >= channel->getUserLimit())
            this->_sendErrorReply(client_fd, ERR_CHANNELISFULL, "#" + channelName + " :Cannot join channel (+l)");
        else if (channel->hasMode(MODE_KEY) && !channel->getKey().empty() && channelPassword != channel->getKey())
            this->_sendErrorReply(client_fd, ERR_BADCHANNELKEY, "#" + channelName + " :Cannot join channel (+k)");
        else
            this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" + channelName + " :Cannot join channel");
        return;
    }
    
    // Add user to channel
    channel->addUser(client->getNickname(), channelPassword);
    client->joinChannel("#" + channelName);
    
    //Announce to everybody on channel that a new client has arrived
    std::string join_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + 
                          client->getHostname() + " JOIN #" + channelName + "\r\n";
    
    
    std::vector<std::string> users = channel->getUserList();
    for (size_t i = 0; i < users.size(); i++)
    {
        Client *user = getClientByNick(users[i]);
        if (user)
        {
            if (send(user->getFd(), join_msg.c_str(), join_msg.length(), 0) == -1)
                logMessage("ERROR: ", RED, "Failed to send JOIN message", YELLOW, ERR);
        }
    }
    
    // Show topic if it exists
    if (!channel->getTopic().empty())
    {
        std::string topic_msg = ":" + _server_name + " 332 " + client->getNickname() + 
                               " #" + channelName + " :" + channel->getTopic() + "\r\n";
        send(client_fd, topic_msg.c_str(), topic_msg.length(), 0);
    }
    
    //Send Names list to everybody on channel (updates list)
    std::string names_list = channel->getUserListString();
    for (size_t i = 0; i < users.size(); i++)
    {
        Client *user = getClientByNick(users[i]);
        if (user)
        {
            std::string names_msg = ":" + _server_name + " 353 " + user->getNickname() + 
                                   " = #" + channelName + " :" + names_list + "\r\n";
            std::string end_names = ":" + _server_name + " 366 " + user->getNickname() + 
                                   " #" + channelName + " :End of /NAMES list\r\n";
            
            send(user->getFd(), names_msg.c_str(), names_msg.length(), 0);
            send(user->getFd(), end_names.c_str(), end_names.length(), 0);
        }
    }
    
    logMessage("User joined channel ", GREEN, channelName + ": " + client->getNickname(), BLUE);
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
                    //this->_sendErrorReply(client_fd, ERR_UNKNOWNMODE, std::string(1, c) + " :is unknown mode char to me");
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
