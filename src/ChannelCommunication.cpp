/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelCommunication.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: crocha-s <crocha-s@student.42.fr>          #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025-08-22 13:57:57 by crocha-s          #+#    #+#             */
/*   Updated: 2025-08-22 13:57:57 by crocha-s         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Channel.hpp"

// Connection and Communication
bool Channel::connect(Server *server, int client_fd)
{
	Client *client = server->getClient(client_fd);
	if (!client)
		return false;

	std::string nick = client->getNickname();

	// Send JOIN confirmation
	std::string join_msg = ":" + nick + "!" + client->getUsername() \
	+ "@" + client->getHostname() + " JOIN #" + this->_name + "\r\n";
	send(client_fd, join_msg.c_str(), join_msg.length(), 0);

	// Send topic if exists
	if (!_topic.empty())
	{
		std::string topic_msg = ":" + server->getServerName() + " 332 " \
		+ nick + " #" + this->_name + " :" + this->_topic + "\r\n";
		send(client_fd, topic_msg.c_str(), topic_msg.length(), 0);
	}
	
	// Send NAMES list
	std::string names_msg = ":" + server->getServerName() + " 353 " \
	+ nick + " = #" + this->_name + " :" + getUserListString() + "\r\n";
	send(client_fd, names_msg.c_str(), names_msg.length(), 0);
	
	std::string end_names = ":" + server->getServerName() + " 366 " \
	+ nick + " #" + this->_name + " :End of /NAMES list\r\n";
	send(client_fd, end_names.c_str(), end_names.length(), 0);

	client->joinChannel("#" + this->_name);
	return true;
}

void Channel::sendMessage(Server *server, const std::string &sender, \
std::string const msg, std::string command)
{
	// Parse sender nickname
	std::string sender_nick = sender;
	size_t exclamation = sender.find('!');
	if (exclamation != std::string::npos)
		sender_nick = sender.substr(0, exclamation);

	std::string formatted_msg = ":" + sender + " " + command \
	+ " #" + _name + " " + msg + "\r\n";

	for (std::set<std::string>::const_iterator it = _userlist.begin(); \
		it != _userlist.end(); ++it)
	{
		// Avoid delivering the message to its own sender
		if (*it == sender_nick)
			continue;

		Client *client = server->getClientByNick(*it);
		if (client)
		{
			if (send(client->getFd(), formatted_msg.c_str(), \
				formatted_msg.length(), 0) == -1)
				logMessage("ERROR: ", RED, "Failed to send message to " \
					+ *it, YELLOW, ERR);
		}
	}
}

// Handle System Messages
void Channel::announceJoin(Server *server, Client *client)
{
	std::string join_msg = ":" + client->getNickname() \
	+ "!" + client->getUsername() + "@" + client->getHostname() \
	+ " JOIN #" + this->_name + "\r\n";
	this->_broadcastToChannel(server, join_msg);
}

void Channel::announcePart(Server *server, Client *client, \
const std::string &reason)
{
	std::string part_msg = ":" + client->getNickname() \
	+ "!" + client->getUsername() + "@" + client->getHostname() \
	+ " PART #" + this->_name;
	if (!reason.empty())
		part_msg += " :" + reason;
	part_msg += "\r\n";

	this->_broadcastToChannel(server, part_msg);
}

void Channel::announceQuit(Server *server, Client *client, \
	const std::string &reason)
{
	std::string quit_msg = ":" + client->getNickname() \
	+ "!" + client->getUsername() + "@" + client->getHostname() \
	+ " QUIT";
	if (!reason.empty())
		quit_msg += " :" + reason;
	quit_msg += "\r\n";

	this->_broadcastToChannel(server, quit_msg, client->getFd());
}

void Channel::announceNickChange(Server *server, const std::string &oldNick, \
	const std::string &newNick)
{
	std::string nick_msg = ":" + oldNick + " NICK :" + newNick + "\r\n";
	this->_broadcastToChannel(server, nick_msg);
}
