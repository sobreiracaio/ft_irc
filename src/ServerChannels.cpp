#include "../include/Server.hpp"

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
