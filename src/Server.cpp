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
	if(command == "NOTICE")
		return NOTICE;
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
	if(command == "PONG")
		return PONG;

	return NO_COMM;
}

bool Server::executeCommand(int client_fd, int command_code, std::string const &data)
{
	Client *client = this->getClient(client_fd);

	if (command_code != QUIT && client) // QUIT does not need activity feedback
		client->setLastActivity(time(NULL));

	switch (command_code)
	{
		case JOIN:	this->joinChannel(data, client_fd); break;
		case PRIVMSG:this->sendMessageToTarget(data, client_fd); break;
		case NOTICE:this->sendMessageToTarget(data, client_fd, NOTICE); break;
		case NICK:	this->changeNick(data, client_fd); break;
		case PART:	this->partChannel(data, client_fd); break;
		case KICK:	this->kickUser(data, client_fd); break;
		case INVITE:this->inviteUser(data, client_fd); break;
		case TOPIC:	this->topicCommand(data, client_fd); break;
		case MODE:	this->modeCommand(data, client_fd); break;
		case PONG:	break;

		case QUIT:
			this->quitServer(data, client_fd);
			return false; // Client removed

		default:
			break;
	}
	return true;
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
		
	if (!channel)
	{
		// Create new channel
		channel = new Channel(channelName, channelPassword);
		this->_channels[channelName] = channel;
	}
	
	// Verify if client can join channel
	if (!channel->canUserJoin(client->getNickname(), channelPassword))
	{
		// Send error based on reason of not joining
		if (channel->hasMode(MODE_INVITE_ONLY) && !channel->isInvited(client->getNickname()))
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
	channel->setTopic(newTopic);
	
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

	// pollfds is vector<pollfd> (structs na stack), então basta limpar
	std::vector<pollfd>().swap(this->_poll_fds);
}
