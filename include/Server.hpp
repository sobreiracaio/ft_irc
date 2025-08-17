/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 16:58:52 by caio              #+#    #+#             */
/*   Updated: 2025/08/17 17:26:28 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <csignal>
#include <cerrno>

#include "Utils.hpp"
#include "Client.hpp"
#include "Channel.hpp"

#define BUFFER_SIZE 4096
#define MAX_NICK_LENGTH 30      
#define MAX_CHANNEL_NAME 50


// IRC COMMAND CODES - ARBITRARY
#define JOIN 100
#define PRIVMSG 101
#define NICK 102
#define QUIT 103
#define PART 104
#define PASS 105
#define USER 106
#define KICK 107
#define INVITE 108
#define TOPIC 109
#define MODE 110
#define PONG 111
#define NO_COMM -1


// IRC REPLY CODE - RFC 1459 PROTOCOL
#define RPL_WELCOME 001
#define RPL_NAMREPLY 353
#define RPL_ENDOFNAMES 366
#define ERR_NOSUCHNICK 401
#define ERR_NOSUCHCHANNEL 403
#define ERR_CANNOTSENDTOCHAN 404
#define ERR_NORECIPIENT 411
#define ERR_NOTEXTTOSEND 412
#define ERR_UNKNOWNCOMMAND 421
#define ERR_NONICKNAMEGIVEN 431
#define ERR_ERRONEUSNICKNAME 432
#define ERR_NICKNAMEINUSE 433
#define ERR_USERNOTINCHANNEL 441
#define ERR_NOTONCHANNEL 442
#define ERR_USERONCHANNEL 443
#define ERR_NEEDMOREPARAMS 461
#define ERR_ALREADYREGISTRED 462
#define ERR_PASSWDMISMATCH 464
#define ERR_CHANNELISFULL 471
#define ERR_UNKNOWNMODE 472
#define ERR_INVITEONLYCHAN 473
#define ERR_BANNEDFROMCHAN 474
#define ERR_BADCHANNELKEY 475
#define ERR_CHANOPRIVSNEEDED 482

class Client;
class Channel;

class Server
{
    private:
    
        int _port;
        std::string _password;
        int _server_fd;
        sockaddr_in _server_addr;
        std::vector<struct pollfd> _poll_fds;
        std::map<int, Client*> _clients;
        std::map<std::string, Channel*> _channels;
        std::string _server_name;
        
        
    
        // Private initialization methods
        bool _checkPassword(std::string const &client_pass);
        int _createSocket(void);
        int _bindSocket(void);
        int _listenSocket(void);

        //Private management methods
        void _acceptNewClient(void);
        void _handleClientData(int client_fd);
        void _removeClient(int client_fd);
        bool _checkGhostClient(std::string data, int client_fd);
        
        //Message handling
        void _sendNumericReply(int client_fd, int code, const std::string &message);
        void _sendErrorReply(int client_fd, int code, const std::string &message);
        void _welcomeMessage(Client *client);
        std::string _checkDoubles (std::string const &nickname, int client_fd);
        
        //Input validation
        bool _isValidNickname(const std::string &nickname);
        bool _isValidChannelName(const std::string &channelName);
        std::vector<std::string> _splitMessage(const std::string &message);
        
        
                      
    
    public:
        Server(int port, std::string password);
        ~Server();

        //Main public methods
        bool serverInit(void);
        void run(void);
        int getServerFd(void);
        std::string getServerName(void) const;
        
        //Client management methods
        Client *getClient(int client_fd);
        Client *getClientByNick(std::string const &nick);
        void changeNick(std::string const &data, int client_fd);
        void privateMsg(std::string const &data, int client_fd);
        void joinChannel(std::string const &data, int client_fd);
        void partChannel(std::string const &data, int client_fd);
        void quitServer(std::string const &data, int client_fd);
        void kickUser(std::string const &data, int client_fd);
        void inviteUser(std::string const &data, int client_fd);
        

        //Channel management methods
        Channel *getChannelByName(std::string const &name);
        void topicCommand(std::string const &data, int client_fd);
        void modeCommand(std::string const &data, int client_fd);
        void handleChannelMode(std::string const &data, int client_fd);
        
        //Server command methods
        int parseCommand(const std::string& data);
        bool executeCommand(int client_fd, int command_code, std::string const &data);

        //Server clean up method
        void cleanUp();
};