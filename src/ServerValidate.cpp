#include "../include/Server.hpp"

bool Server::_checkPassword(std::string const &client_pass)
{
	return (this->_password == client_pass);
}

bool Server::_isValidNickname(const std::string &nickname)
{
	if (nickname.empty() || nickname.length() > MAX_NICK_LENGTH)
		return false;
	
	// First char must be a letter or a valid special char
	char first = nickname[0];
	if (!isalpha(first) && first != '[' && first != ']' && first != '\\' && 
		first != '`' && first != '_' && first != '^' && first != '{' && first != '}')
		return false;

	// Other characters can be alphanumericals or special chars
	for (size_t i = 1; i < nickname.length(); i++)
	{
		char c = nickname[i];
		if (!isalnum(c) && c != '[' && c != ']' && c != '\\' && 
			c != '`' && c != '_' && c != '^' && c != '{' && c != '}' && c != '-')
			return false;
	}
	return true;
}

bool Server::_isValidChannelName(const std::string &channelName)
{
	if (channelName.empty() || channelName.length() > MAX_CHANNEL_NAME)
		return false;

	// Channel name cannot contain spaces, commas or control chars
	for (size_t i = 0; i < channelName.length(); i++)
	{
		char c = channelName[i];
		if (c == ' ' || c == ',' || c == '\a' || c == '\0' \
			|| c == '\r' || c == '\n')
			return false;
	}
	return true;
}

std::string Server::_checkDoubles(std::string const &nickname, int client_fd)
{
	std::string modifiedNickname = nickname;
	
	// Remove control characters
	std::string cleanNick;
	for (size_t i = 0; i < modifiedNickname.length(); i++)
	{
		unsigned char c = static_cast<unsigned char>(modifiedNickname[i]);
		// Remove every control character except normal spaces
		if (c >= 32 && c != 127) // Printable characters
			cleanNick += c;
		else if (c == ' ') // Allows space, to be handled later
			break; //Stops on first space
	}
	modifiedNickname = cleanNick;
	
	// Remove espaços extras
	size_t space_pos = modifiedNickname.find(' ');
	if (space_pos != std::string::npos)
		modifiedNickname.erase(space_pos);
	
	if (!_isValidNickname(modifiedNickname))
	{
		this->_sendErrorReply(client_fd, ERR_ERRONEUSNICKNAME, "Erroneous nickname");
		return modifiedNickname;
	}
	
	// Verificação melhorada para nicks duplicados
	std::map<int, Client*>::iterator it;
	for (it = this->_clients.begin(); it != this->_clients.end(); it++)
	{
		Client *client = it->second;
		// Verifica se o cliente existe, está ativo e tem o mesmo nick
		if (client && client->getFd() != client_fd && !client->getNickname().empty() && 
			client->getNickname() == modifiedNickname)
		{
			this->_sendErrorReply(client_fd, ERR_NICKNAMEINUSE, modifiedNickname + " :Nickname is already in use");
			modifiedNickname += "_";
			break;
		}
	}
	return modifiedNickname;
}
