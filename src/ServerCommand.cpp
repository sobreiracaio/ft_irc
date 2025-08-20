#include "../include/Server.hpp"

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
