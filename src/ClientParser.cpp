#include "../include/Client.hpp"

void Client::parsePassCommand(const std::string &line)
{
	if (line.length() > 5)
	{
		std::string password = line.substr(5);
		
		// Remove spaces from the beginning
		size_t start = password.find_first_not_of(" \t");
		if (start != std::string::npos)
			password = password.substr(start);
		
		// Remove control characters from the end (but keeps content)
		size_t end = password.find_last_not_of(" \t\r\n");
		if (end != std::string::npos)
			password = password.substr(0, end + 1);
		
		this->_password = password;
		this->_hasPassword = !password.empty();
		
		logMessage("Password set for client FD=" + itoa(_client_fd) \
				+ ": ", CYAN, (password.empty() ? "[EMPTY]" \
				: this->_password), WHITE);

		this->checkRegistrationComplete();
	}
}

void Client::parseNickCommand(const std::string &line)
{
	if (line.length() > 5)
	{
		std::string nickname = line.substr(5);
		
		// Remove spaces from beginning
		size_t start = nickname.find_first_not_of(" \t");
		if (start != std::string::npos)
			nickname = nickname.substr(start);
		
		// Remove spaces and control characters from the end
		size_t end = nickname.find_last_not_of(" \t\r\n");
		if (end != std::string::npos)
			nickname = nickname.substr(0, end + 1);
		
		// Remove spaces in the middle to get the nickname
		size_t space_pos = nickname.find(' ');
		if (space_pos != std::string::npos)
			nickname = nickname.substr(0, space_pos);
		
		this->setNickname(nickname);
		logMessage("Nickname parsed for client FD=" + itoa(_client_fd) \
			+ ": ", CYAN, nickname, WHITE);
	}
}

void Client::parseUserCommand(const std::string &line)
{
	if (line.length() > 5)
	{
		std::string user_data = line.substr(5);
		std::istringstream iss(user_data);
		std::string username, mode, unused;

		// Format: USER <username> <mode> <unused> :<realname>
		if (iss >> username >> mode >> unused)
		{
			this->setUsername(username);

			// Gets the rest of the line as realname (after ":")
			std::string remaining;
			std::getline(iss, remaining);

			size_t colon_pos = remaining.find(':');
			if (colon_pos != std::string::npos)
			{
				std::string realname = remaining.substr(colon_pos + 1);

				// Remove spaces from beginning and end of the realname
				size_t start = realname.find_first_not_of(" \t");
				if (start != std::string::npos)
					realname = realname.substr(start);
				
				size_t end = realname.find_last_not_of(" \t\r\n");
				if (end != std::string::npos)
					realname = realname.substr(0, end + 1);
				this->setRealname(realname);
			}

			logMessage("User info parsed for client FD=" + itoa(_client_fd) \
				+ ": ", CYAN, username, WHITE);
		}
	}
}
