#include "../include/Server.hpp"

Server::Server(int port, std::string password): _port(port), _password(password), _server_fd(-1)
{
	this->_server_name = "ircserv";
}

Server::~Server()
{	
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
	
	// Poll setup for server socket
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
		time_t now = time(NULL); // Time in seconds
		int poll_count = poll(&this->_poll_fds[0], this->_poll_fds.size(), 5000);
		if (poll_count == -1)
		{
			logMessage("ERROR: ", RED, "Poll failed!", YELLOW, ERR);
			break;
		}
		if(poll_count == 0 && this->_poll_fds.size() > 1)
		{
			for(size_t i = 1; i < this->_poll_fds.size(); i++)
			{
				Client *client = this->getClient(this->_poll_fds[i].fd);
				
				if(now - client->getLastActivity() > 300) // 5 minutes idle is automatically disconected
					this->quitServer("QUIT", client->getFd(), "Idle");
			}
		}
		size_t poll_size = this->_poll_fds.size();
		for (size_t i = 0; i < poll_size; i++)
		{
			if (i >= this->_poll_fds.size())
				break;
			if(this->_poll_fds[i].revents & POLLIN)
			{
				if(this->_poll_fds[i].fd == this->_server_fd)
				{
					this->_acceptNewClient();
				}
				else
				{
					this->_handleClientData(this->_poll_fds[i].fd);
					if(i >= this->_poll_fds.size())
						break;
				}
			}
			else if(this->_poll_fds[i].revents & (POLLHUP | POLLERR))
			{
				//Client disconnected by socket error
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

std::vector<std::string> Server::_splitMessage(const std::string &message)
{
	std::vector<std::string> result;
	std::istringstream iss(message);
	std::string token;

	while (iss >> token)
		result.push_back(token);
	return result;
}

void Server::_sendErrorReply(int client_fd, int code, const std::string &message)
{
	Client *client = getClient(client_fd);
	if (!client)
		return;

	std::ostringstream oss;
	oss << ":" << _server_name << " " << std::setfill('0') << std::setw(3) << code << " " << (client->getNickname().empty() ? "*" : client->getNickname()) << " :" << message << "\r\n";

	std::string reply = oss.str();
	if (send(client_fd, reply.c_str(), reply.length(), 0) == -1)
		logMessage("ERROR: ", RED, "Failed to send error reply!", YELLOW, ERR);
}

int Server::getServerFd(void)
{
	return this->_server_fd;
}

std::string Server::getServerName(void) const
{
	return (this->_server_name);
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

void Server::sendMessageToTarget(std::string const &data, int client_fd, int type)
{
	std::string command = (type == PRIVMSG) ? "PRIVMSG" : "NOTICE";
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
		channel->sendMessage(this, sender_prefix, message, command);
		
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
								sender->getHostname() + " " + command + " " + target + " " + message + "\r\n";
		
		if (send(receiver->getFd(), returnMsg.c_str(), returnMsg.length(), 0) == -1)
			logMessage("ERROR: ", RED, "Failed to send private message!", YELLOW, ERR);
	}
}

void noticeMsg (std::string const &data, int client_fd);

void Server::_welcomeMessage(Client* client)
{
	std::string user = "" + client->getNickname() + "";
	std::stringstream welcome;

	// Largura total da moldura sem os dois '#'
	const int totalWidth = 70;

	// Calcula espaços para centralizar o usuário
	int padding = totalWidth - static_cast<int>(user.size());
	if (padding < 0) padding = 0;
	int padLeft = padding / 2;
	int padRight = padding - padLeft;

	welcome << "########################################################################\n";
	welcome << "#                         Bem-vindo ao IRC Server                      #\n";
	welcome << "#" << std::string(padLeft, ' ') << user << std::string(padRight, ' ') << "#\n";
	welcome << "#  Comandos disponíveis:                                               #\n";
	welcome << "#                                                                      #\n";
	welcome << "#  /JOIN   #canal [senha]       -> Entrar em um canal                  #\n";
	welcome << "#  /PRIVMSG alvo :mensagem      -> Enviar mensagem privada             #\n";
	welcome << "#  /NOTICE alvo mensagem       -> Enviar notificação                   #\n";
	welcome << "#           OBS: alvo pode ser user ou #canal                          #\n";
	welcome << "#  /NICK   novonick             -> Alterar seu apelido                 #\n";
	welcome << "#  /QUIT                        -> Sair do servidor                    #\n";
	welcome << "#                                                                      #\n";
	welcome << "#  Comandos de canal:                                                  #\n";
	welcome << "#  /KICK   #canal nick [motivo] -> Expulsar usuário                    #\n";
	welcome << "#  /INVITE nick #canal          -> Convidar usuário                    #\n";
	welcome << "#  /TOPIC  #canal [tópico]      -> Definir/ver tópico                  #\n";
	welcome << "#  /MODE   #canal +i/-i         -> Modo convidado obrigatório/livre    #\n";
	welcome << "#  /MODE   #canal +t/-t         -> Apenas operadores mudam tópico      #\n";
	welcome << "#  /MODE   #canal +k/-k senha   -> Definir/remover senha               #\n";
	welcome << "#  /MODE   #canal +o/-o nick    -> Dar/remover operador                #\n";
	welcome << "#  /MODE   #canal +l/-l [n]     -> Definir/remover limite              #\n";
	welcome << "########################################################################\n";  

	
	std::string line;

	while(getline(welcome, line, '\n'))
	{
		std::string msg = ":" + client->getHostname() + " 001 " + client->getNickname() + " :" + line + "\r\n";
		send(client->getFd(), msg.c_str(), msg.length(), 0);
	}
}

void Server::cleanUp()
{
	// Free all clients
	for (std::map<int, Client*>::iterator it = this->_clients.begin();
		 it != this->_clients.end(); ++it) {
		delete it->second;   //Deletes the second on the ::map, which is Client*
	}
	this->_clients.clear();

	//Free all channels
	for (std::map<std::string, Channel*>::iterator it = this->_channels.begin();
		 it != this->_channels.end(); ++it) {
		delete it->second;
	}
	this->_channels.clear();

	std::vector<pollfd>().swap(this->_poll_fds);
	// std::vector<pollfd>() creates a temporary, empty vector and .swap(this->_poll_fds) swaps the contents of the empty vector with this->_poll_fds. The old data is now in the temporary vector, which is immediately destroyed, freeing its memory.
}
