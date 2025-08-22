/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientSet.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: crocha-s <crocha-s@student.42.fr>          #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025-08-22 13:58:19 by crocha-s          #+#    #+#             */
/*   Updated: 2025-08-22 13:58:19 by crocha-s         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"

void Client::setNickname(const std::string &nickname)
{
	std::string formattedNick = nickname; 

	// Remove control chars
	size_t pos = formattedNick.find_first_of("\r\n");
	if (pos != std::string::npos)
		formattedNick.erase(pos);

	// Remove extra spaces
	pos = formattedNick.find(' ');
	if (pos != std::string::npos)
		formattedNick.erase(pos);

	this->_nickname = formattedNick;
	this->_hasNick = !formattedNick.empty();
	this->checkRegistrationComplete();
}

void Client::setUsername(const std::string &username)
{
	this->_username = username;
	this->_hasUser = !username.empty();
	this->checkRegistrationComplete();
}

void Client::setRealname(const std::string &realname)
{
	this->_realname = realname;
}

void Client::setNamesAndPass(std::string const &data)
{
	std::istringstream iss(data);
	std::string line;

	while(std::getline(iss, line))
	{
		// Remove \r if its present
		if(!line.empty() && line[line.length() - 1] == '\r') \
			line.erase(line.length() - 1);
		
		if (line.empty())
			continue;

		// Parse different register commands
		if (line.length() >= 5)
		{
			std::string cmd = line.substr(0, 5);
			if(cmd == "PASS ")
				this->parsePassCommand(line);
			else if(cmd == "NICK ")
				this->parseNickCommand(line);
			else if(cmd == "USER ")
				this->parseUserCommand(line);
		}
	}
}
void Client::setLastActivity (time_t now)
{
	this->_lastActivity = now;
}
