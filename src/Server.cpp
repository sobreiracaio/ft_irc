/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 17:13:07 by caio              #+#    #+#             */
/*   Updated: 2025/08/06 19:06:16 by caio             ###   ########.fr       */
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
            std::cout << "Client buffer is: " << client->getBuffer() << std::endl;
            int command_code = this->parseCommand(data);
            this->executeCommand(client_fd, command_code, data);
            client->cleanBuffer();
        }
        
        // CONTINUE PARSING DATA
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
    std::cout << temp << std::endl;
    if(temp == "JOIN")
    {
        logMessage(temp, RED," Code is: " + itoa(JOIN), YELLOW);
        return (JOIN);
    }
    if(temp == "PRIVMSG")
    {
        logMessage(temp, RED," Code is: " + itoa(PRIVMSG), YELLOW);
        return (PRIVMSG);
    }
    if(temp == "NICK")
    {
        logMessage(temp, RED," Code is: " + itoa(NICK), YELLOW);
        return (NICK);
    }
    
    return (NO_COMM);                
}

void Server::executeCommand(int client_fd, int command_code, std::string const &data)
{
    switch (command_code)
    {
    case JOIN:
        /* code */
        break;
    case PRIVMSG:
        /* code */
        break;
    case NICK:
        this->changeNick(data, client_fd);
        break;
    
        
    default:
        break;
    }
    return;
}

void Server::changeNick(std::string const &data, int client_fd)
{
    Client *tempClient = this->_clients[client_fd];

    std::string nickname = data.substr(5);
    std::string old_nick = tempClient->getNickname();
    if(old_nick[old_nick.length() - 1] == '\n' || old_nick[old_nick.length() - 1] == '\r')
        old_nick.pop_back();
    std::string msg = ":" + old_nick + "!" + tempClient->getUsername() + "@" + tempClient->getHostname() + " " +
                        "NICK :" + nickname + "\r\n";
        
    std::cout <<"FD = "<< tempClient->getFd() << " - " << tempClient->getNickname()<<" --> " << msg << std::endl;                    
    send(tempClient->getFd(), msg.c_str(), msg.length(), 0);
    tempClient->setNickname(nickname);
    //delete tempClient;
    
}

