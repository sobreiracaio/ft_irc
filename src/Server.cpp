/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/08/11 18:45:20 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(int port, std::string password)
{
    this->_port = port;
    this->_password = password;
}

Server::~Server()
{
    std::map<int, Client*>::iterator it;
    for (it = this->_clients.begin(); it != this->_clients.end(); it++)
        delete it->second;
    this->_clients.clear();
    
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
    setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    logMessage("Server socket created successfully!", BLUE, "", RESET);
    this->_server_fd = listen_socket;
    return (this->_server_fd);
}

int Server::_bindSocket(void)
{
    memset(&this->_server_addr, 0, sizeof(this->_server_addr));
    this->_server_addr.sin_family = AF_INET;
    this->_server_addr.sin_port = htons(this->_port);
    inet_pton(AF_INET, "0.0.0.0", &this->_server_addr.sin_addr);
 
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

        for (size_t i = 0; i < this->_poll_fds.size(); i++)
        {
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
        logMessage("ERROR: ", RED, "Accept failed!", YELLOW, ERR);
        return;
    }
    
    fcntl(client_socket, F_SETFL, O_NONBLOCK);
    
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
    std::string data(buffer, bytes_received);
    logMessage("from FD = " + itoa(client_fd) + ":\n", BLUE, data, WHITE);
    
    if(client->getNickname().empty())
    {
        client->setNamesAndPass(data);
        std::string nick = this->_checkDoubles(client->getNickname(), client_fd);
        client->setNickname(nick);
        this->_welcomeMessage(client);
        return;
    }
    
    if(client->getPassword() != this->_password)
    {
        logMessage("Client disconnected! FD = ", RED, "Invalid Password!", YELLOW);
        this->_removeClient(client_fd);
        return;
    }

    
    
    if (bytes_received <= 0)
    {
        logMessage("Client disconnected! FD = ", RED, itoa(client_fd), YELLOW);
        this->_removeClient(client_fd);
    }
    else
    {
        client->appendBuffer(data);
        if(client->isDataComplete())
        {
            //std::cout << "Client buffer is: " << client->getBuffer() << std::endl;
            int command_code = this->parseCommand(data);
            this->executeCommand(client_fd, command_code, data);
            client->cleanBuffer();
        }
        
        
    }
    
}

void Server::_removeClient(int client_fd)
{
    std::vector<struct pollfd>::iterator it;
    std::map<int, Client*>::iterator client_it;

    client_it = this->_clients.find(client_fd);

    for(it = this->_poll_fds.begin(); it != this->_poll_fds.end(); it++)
    {
        if(it->fd == client_fd)
        {
            this->_poll_fds.erase(it);
            break;
        }   
    }
    if (client_it != this->_clients.end())
    {
        delete client_it->second;
        this->_clients.erase(client_it);
    }

    close(client_fd);
    logMessage("Client removed! FD = ", RED, itoa(client_fd), YELLOW);
    
}

Client *Server::getClient(int client_fd)
{
    std::map<int, Client*>::iterator it;
    it = this->_clients.find(client_fd);
    if(it != this->_clients.end())
        return it->second;
    else
        return (NULL);
}

int Server::getServerFd(void)
{
    return this->_server_fd;
}

bool Server::_checkPassword(std::string const &client_pass)
{
    if(this->_password != client_pass)
        return false;
    return true;
}

int Server::parseCommand(const std::string& data)
{
    std::istringstream iss(data);
    std::string temp;
    
    std::getline(iss, temp, ' ');
    if(temp == "JOIN")
        return (JOIN);
    if(temp == "PRIVMSG")
        return (PRIVMSG);
    if(temp == "NICK")
        return (NICK);
    
        
    return (NO_COMM);                
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
    
        
    default:
        break;
    }
    return;
}

std::string Server::_checkDoubles(std::string const &nickname, int client_fd)
{
    std::string modifiedNickname = nickname;
    size_t pos = modifiedNickname.find('\r');
    std::map<int, Client*>::iterator it;

    if(pos && pos != std::string::npos)
        modifiedNickname.erase(pos, 2);
    
    for (it = this->_clients.begin(); it != this->_clients.end(); it++)
    {
        Client *client = this->getClient(it->first);
         
        if((client->getNickname() == modifiedNickname) && client->getFd() != client_fd)
        {
            std::string msg = "Server: " + client->getHostname() +", Nickname: " + modifiedNickname + " is already in use!" + "\r\n";
            modifiedNickname += "_";
            send(client_fd, msg.c_str(), msg.length(), 0);
        }
    }
    return (modifiedNickname);
}

void Server::changeNick(std::string const &data, int client_fd)
{
    Client *tempClient = this->_clients[client_fd];
    
    std::string nickname = this->_checkDoubles(data.substr(5), client_fd);
    
    std::string old_nick = tempClient->getNickname();
    if(old_nick[old_nick.length() - 1] == '\n' || old_nick[old_nick.length() - 1] == '\r')
        old_nick.pop_back();
    std::string msg = ":" + old_nick + "!" + tempClient->getUsername() + "@" + tempClient->getHostname() + " " +
                        "NICK :" + nickname + "\r\n";
        
    std::cout <<"FD = "<< tempClient->getFd() << " - " << tempClient->getNickname()<<" --> " << msg << std::endl;                    
    send(tempClient->getFd(), msg.c_str(), msg.length(), 0);
    tempClient->setNickname(nickname);
   
    
}

void Server::privateMsg(std::string const &data, int client_fd)
{
    Client *sender = getClient(client_fd);
    
    size_t space = data.find(' ');
    size_t colon = data.find(':', space);
    std::string err = ":ircserv 411 " + sender->getNickname() + " No recipient given (PRIVMSG)\r\n";
    
    if(space == std::string::npos || colon == std::string::npos)
    {
        send(client_fd, err.c_str(), err.length(), 0);
        return;
    }
        
    std::string target = data.substr(space + 1, colon - space - 1);
    std::string message = data.substr(colon + 1);
    
    if(target.empty())
    {
        send(client_fd, err.c_str(), err.length(), 0);
        return;
    }
    
    if (target[0] == '#')
    {
        std::string no_hash = target.substr(1);
        size_t pos = no_hash.find(" ");

        if(pos != std::string::npos)
            no_hash.erase(pos);

        
        if(this->getChannelByName(no_hash)) // verify if the channel exists
        {
             
        //     // send message to channel except for the sender (comparar o numero do client_fd)
        //     return;
        }
        else
        {
            std::cout << "nao existe esse canal" << std::endl;
            //message telling that the channel doesnt exist
        }
    }
    else
    {
        Client *receiver = this->getClientByNick(target);
        if(!receiver)
        {
            send(client_fd, err.c_str(), err.length(), 0);
            return;
        }
        std::string returnMsg = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" +
                            sender->getHostname() + " PRIVMSG " + target + message + "\r\n";
        
        std::cout << returnMsg << std::endl;
        send(receiver->getFd(), returnMsg.c_str(), returnMsg.length(), 0);
    }
    
}

Client *Server::getClientByNick(std::string const &nick)
{
    std::map<int, Client*>::iterator client_it;
    
    std::string formattedNick = nick;
    std::string::size_type pos;

    pos = formattedNick.find(" ");
    if (pos != std::string::npos)
        formattedNick.erase(pos);
    


    for (client_it = this->_clients.begin(); client_it != this->_clients.end(); client_it++)
    {
        Client *temp = client_it->second;
        
        if(temp->getNickname() == formattedNick)
            return (temp);
    }
    return (NULL);
}

void Server::joinChannel(std::string const &data, int client_fd)
{
    std::string channelName;
    std::string channelPassword = "";
    
    std::string unfilteredData = data.substr(5);
    size_t pos = unfilteredData.find("\r\n");
    
    if(pos != std::string::npos)
        unfilteredData.erase(pos, 2);     
  
    std::istringstream iss(unfilteredData);

    iss >> channelName >> channelPassword;

    channelName = channelName.substr(1);
    
    if(this->_channels.empty()) // caso ainda nao haja canais 
    {
        Channel *new_channel = new Channel(channelName, channelPassword);
        this->_channels[channelName] = new_channel;
        new_channel->addUser(this->getClient(client_fd)->getNickname());
        std::cout << "AQUI 1" << std::endl;
        new_channel->connect(this, client_fd);
    }
    else if(this->getChannelByName(channelName) == NULL) // caso o canal nao esteja na lista
    {
        Channel *new_channel = new Channel(channelName, channelPassword);
        this->_channels[channelName] = new_channel;
        new_channel->addUser(this->getClient(client_fd)->getNickname());
        std::cout << "AQUI 2" << std::endl;
        new_channel->connect(this, client_fd);
    }
    else 
    {
        Channel *existingChannel = this->getChannelByName(channelName);
        existingChannel->addUser(this->getClient(client_fd)->getNickname());
        std::cout << "AQUI 3" << std::endl;
        
        existingChannel->connect(this, client_fd);
        //usar os metodos de existing channel para criar a mensagem formatada irc para o client interpretar
        // se o canal estiver na lista entra nele
    }
    

    
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

void Server::_removeChannel(std::string const &channel_name)
{
   
}
