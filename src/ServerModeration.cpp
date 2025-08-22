/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerModeration.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: crocha-s <crocha-s@student.42.fr>          #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025-08-22 13:59:14 by crocha-s          #+#    #+#             */
/*   Updated: 2025-08-22 13:59:14 by crocha-s         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

void Server::kickUser(std::string const &data, int client_fd)
{
	std::vector<std::string> tokens = _splitMessage(data);
	
	if (tokens.size() < 3)
	{
		this->_sendErrorReply(client_fd, ERR_NEEDMOREPARAMS, \
			"KICK :Not enough parameters");
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
		this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" \
			+ channelName + " :No such channel");
		return;
	}
	
	Client *kicker = getClient(client_fd);
	if (!kicker)
		return;
	
	// Check if kicker is in channel and is operator
	if (!channel->hasUser(kicker->getNickname()))
	{
		this->_sendErrorReply(client_fd, ERR_NOTONCHANNEL, "#" \
			+ channelName + " :You're not on that channel");
		return;
	}
	
	if (!channel->isOp(kicker->getNickname()))
	{
		this->_sendErrorReply(client_fd, ERR_CHANOPRIVSNEEDED, "#" \
			+ channelName + " :You're not channel operator");
		return;
	}
	
	// Check if target exists and is in channel
	Client *target = getClientByNick(targetNick);
	if (!target)
	{
		this->_sendErrorReply(client_fd, ERR_NOSUCHNICK, targetNick \
			+ " :No such nick/channel");
		return;
	}
	
	if (!channel->hasUser(targetNick))
	{
		this->_sendErrorReply(client_fd, ERR_USERNOTINCHANNEL, targetNick \
			+ " #" + channelName + " :They aren't on that channel");
		return;
	}
	
	// Send KICK message to all channel members
	std::string kick_msg = ":" + kicker->getNickname() + "!" + kicker->getUsername() + "@" + kicker->getHostname() + " KICK #" \
		+ channelName  + " " + targetNick + " :" + reason + "\r\n";
	
	// Broadcast to channel
	std::vector<std::string> users = channel->getUserList();
	for (size_t i = 0; i < users.size(); i++)
	{
		Client *user = getClientByNick(users[i]);
		if (user)
		{
			if (send(user->getFd(), kick_msg.c_str(), kick_msg.length(), 0) == -1)
				logMessage("ERROR: ", RED, \
					"Failed to send KICK message", YELLOW, ERR);
		}
	}
	
	// Remove user from channel
	channel->removeUser(targetNick);
	target->leaveChannel("#" + channelName);
	
	logMessage("User kicked from channel ", YELLOW, channelName + \
		": " + targetNick, RED);
}

void Server::inviteUser(std::string const &data, int client_fd)
{
	std::vector<std::string> tokens = _splitMessage(data);
	
	if (tokens.size() < 3)
	{
		this->_sendErrorReply(client_fd, ERR_NEEDMOREPARAMS, \
			"INVITE :Not enough parameters");
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
		this->_sendErrorReply(client_fd, ERR_NOSUCHCHANNEL, "#" \
			+ channelName + " :No such channel");
		return;
	}
	
	// Check if inviter is in channel
	if (!channel->hasUser(inviter->getNickname()))
	{
		this->_sendErrorReply(client_fd, ERR_NOTONCHANNEL, "#" \
			+ channelName + " :You're not on that channel");
		return;
	}
	
	// Check if inviter has permission (if channel is +i, only ops can invite)
	if (channel->hasMode(MODE_INVITE_ONLY) && !channel->isOp(inviter->getNickname()))
	{
		this->_sendErrorReply(client_fd, ERR_CHANOPRIVSNEEDED, "#" \
			+ channelName + " :You're not channel operator");
		return;
	}
	
	Client *target = getClientByNick(targetNick);
	if (!target)
	{
		this->_sendErrorReply(client_fd, ERR_NOSUCHNICK, targetNick \
			+ " :No such nick/channel");
		return;
	}
	
	// Check if target is already in channel
	if (channel->hasUser(targetNick))
	{
		this->_sendErrorReply(client_fd, ERR_USERONCHANNEL, targetNick \
			+ " #" + channelName + " :is already on channel");
		return;
	}
	
	// Adds target to invite list
	channel->inviteUser(targetNick);
	
	// Sends invite to target
	std::string invite_msg = ":" + inviter->getNickname() + "!" \
	+ inviter->getUsername() + "@" + inviter->getHostname() + " INVITE " \
	+ targetNick + " #" + channelName + "\r\n";
	
	if (send(target->getFd(), invite_msg.c_str(), invite_msg.length(), 0) == -1)
		logMessage("ERROR: ", RED, "Failed to send INVITE", YELLOW, ERR);
	
	// Sends invite confirmation to inviter
	std::string confirm_msg = ":" + _server_name + " 341 " \
	+ inviter->getNickname() + " " + targetNick + " #" + channelName + "\r\n";
	
	if (send(client_fd, confirm_msg.c_str(), confirm_msg.length(), 0) == -1)
		logMessage("ERROR: ", RED, \
			"Failed to send INVITE confirmation", YELLOW, ERR);
	
	logMessage("User invited to channel ", GREEN, \
		channelName + ": " + targetNick, BLUE);
}
