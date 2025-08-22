/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: crocha-s <crocha-s@student.42.fr>          #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025-08-22 13:57:05 by crocha-s          #+#    #+#             */
/*   Updated: 2025-08-22 13:57:05 by crocha-s         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sstream>
#include <vector>
#include <set>
#include <ctime>

#include "Utils.hpp"

#define MAX_BUFFER_SIZE 4096

class Client
{
	private:
		int _client_fd;
		sockaddr_in _client_addr;
		std::string _nickname;
		std::string _username;
		std::string _realname;
		std::string _hostname;
		std::string _password;
		std::string _buffer;
		std::set<std::string> _channels; //Channels in which the client is participating
		time_t _lastActivity;
		bool _isRegistered;
		bool _hasPassword;
		bool _hasNick;
		bool _hasUser;

	public:
		Client(int client_socket, sockaddr_in client_addr);
		~Client();
		
		//Getters
		int getFd() const;
		std::string getNickname() const;
		std::string getUsername() const;
		std::string getRealname() const;
		std::string getHostname() const;
		std::string getPassword() const;
		std::string getBuffer() const;
		sockaddr_in getClientAddr() const;
		bool isRegistered() const;
		
		//Setters
		void setNickname(const std::string &nickname);
		void setUsername(const std::string &username);
		void setRealname(const std::string &realname);
		
		//Buffer Management
		void appendBuffer(const std::string &data);
		bool isDataComplete() const;
		void cleanBuffer();
		std::string getNextCompleteMessage();

		//Registration process
		void setNamesAndPass(const std::string &data);
		void checkRegistrationComplete();

		//Channel Management
		void joinChannel(const std::string &channelName);
		void leaveChannel(const std::string &channelName);
		bool isInChannel(const std::string &channelName) const;
		std::set<std::string> getChannels() const;
		
		// Validation
		bool isValidInput(const std::string &input) const;

		//Parsing
		void parsePassCommand(const std::string &line);
		void parseNickCommand(const std::string &line);
		void parseUserCommand(const std::string &line);

		//Activity check
		void setLastActivity (time_t now);
		time_t getLastActivity (void);
};
