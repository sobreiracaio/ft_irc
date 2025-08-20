#include "../include/Channel.hpp"

Channel::Channel(std::string const &name, std::string const &password) \
				: _name(name), _password(password), _key(""), _user_limit(0)
{
	this->_creation_time = time(NULL);
	// Default modes for new channels
	this->_modes.insert(MODE_NO_EXTERNAL_MSGS);	// +n by default
	this->_modes.insert(MODE_TOPIC_PROTECT);	// +t by default
	logMessage("Channel created: ", GREEN, name, BLUE);
}

Channel::~Channel()
{
	logMessage("Channel destroyed: ", RED,this->_name, YELLOW);
}

// Getters
std::string Channel::getName() const
{
	return this->_name;
}

std::string Channel::getTopic() const
{
	return this->_topic;
}

std::string Channel::getPassword() const
{
	return this->_password;
}

std::string Channel::getKey() const
{
	return this->_key;
}

size_t Channel::getUserCount() const
{
	return this->_userlist.size();
}

size_t Channel::getUserLimit() const
{
	return this->_user_limit;
}

time_t Channel::getCreationTime() const
{
	return this->_creation_time;
}

// Setters
void Channel::setTopic(std::string const &topic)
{
	this->_topic = topic;
	logMessage("Topic set for channel ", BLUE,this->_name + ": " \
		+ topic, GREEN);
}

// User management
bool Channel::addUser(std::string const &user, std::string const &password)
{
	// Remove spaces and control characters from username
	std::string cleanUser = user;
	size_t pos = cleanUser.find_first_of(" \r\n\t");
	if (pos != std::string::npos)
		cleanUser.erase(pos);

	if (cleanUser.empty())
		return false;

	if (this->_userlist.find(cleanUser) != this->_userlist.end())
		return true; // Not an error: the user is already on the channel

	if (!canUserJoin(cleanUser, password))
		return false;
	this->_userlist.insert(cleanUser);

	// First user becomes operator
	if (this->_userlist.size() == 1)
		this->_operators.insert(cleanUser);

	// Remove from invite list if present
	this->_invited.erase(cleanUser);

	logMessage("User joined channel ", GREEN, this->_name + ": " + cleanUser, BLUE);
	return true;
}

bool Channel::removeUser(std::string const &user)
{
	if (this->_userlist.find(user) == this->_userlist.end())
		return false;
	
	this->_userlist.erase(user);
	this->_operators.erase(user);

	logMessage("User left channel ", YELLOW,this->_name + ": " + user, BLUE);
	return true;
}

bool Channel::hasUser(std::string const &user) const
{
	return (this->_userlist.find(user) != this->_userlist.end());
}

std::vector<std::string> Channel::getUserList() const
{
	std::vector<std::string> users;
	for (std::set<std::string>::const_iterator it = this->_userlist.begin(); \
		it != this->_userlist.end(); ++it)
		users.push_back(*it);
	return users;
}

std::string Channel::getUserListString() const
{
	std::string result;
	for (std::set<std::string>::const_iterator it = this->_userlist.begin(); \
		it != this->_userlist.end(); ++it)
	{
		if (!result.empty())
			result += " ";
		result += this->_getUserModePrefix(*it) + *it;
	}
	return result;
}

// Handle invitation
bool Channel::inviteUser(std::string const &user)
{
	this->_invited.insert(user);
	logMessage("User invited to ", GREEN,this->_name + ": " + user, BLUE);
	return true;
}

bool Channel::isInvited(std::string const &user) const
{
	return (this->_invited.find(user) != this->_invited.end());
}

// Validations
bool Channel::isPasswordRequired() const
{
	return !this->_password.empty();
}

bool Channel::canUserJoin(std::string const &user, \
	std::string const &password) const
{
	// Check password
	if (isPasswordRequired() && password != this->_password)
		return false;

	// Check invite-only mode
	if (hasMode(MODE_INVITE_ONLY) && !isInvited(user))
		return false;

	// Check user limit
	if (hasMode(MODE_LIMIT) &&this->_user_limit > 0 \
		&& this->_userlist.size() >= this->_user_limit)
		return false;

	// Check key mode
	if (hasMode(MODE_KEY) && !_key.empty() && password != this->_key)
		return false;

	return true;
}

bool Channel::canUserSetTopic(std::string const &user) const
{
	if (!hasUser(user))
		return false;

	// If topic protection is on, only operators can set topic
	if (hasMode(MODE_TOPIC_PROTECT))
		return isOp(user);

	return true;
}

// Empty channel checking
bool Channel::isEmpty() const
{
	return this->_userlist.empty();
}

// Private methods
bool Channel::_hasMode(char mode) const
{
	return (this->_modes.find(mode) != this->_modes.end());
}

void Channel::_boreadcastToChannel(Server *server, const std::string &message, int exclude_fd)
{
	for (std::set<std::string>::const_iterator it = this->_userlist.begin(); it != this->_userlist.end(); ++it)
	{
		Client *client = server->getClientByNick(*it);
		if (client && client->getFd() != exclude_fd)
		{
			if (send(client->getFd(), message.c_str(), message.length(), 0) == -1)
				logMessage("ERROR: ", RED, "Failed to broadcast to " + *it, YELLOW, ERR);
		}
	}
}

std::string Channel::_getUserModePrefix(const std::string &nickname) const
{
	if (isOp(nickname))
		return "@";
	return "";
}

bool Channel::updateUserNick(const std::string &oldNick, const std::string &newNick)
{
	if (this->_userlist.find(oldNick) == this->_userlist.end())
		return false; // User is not in the channel

	this->_userlist.erase(oldNick); // Removes old nickname
	this->_userlist.insert(newNick); // Adds new nickname

	// Updates operator nickname
	if (this->_operators.find(oldNick) != this->_operators.end())
	{
		this->_operators.erase(oldNick);
		this->_operators.insert(newNick);
	}
	
	// Removes old nickname from banned and invited lists
	this->_banned.erase(oldNick);
	this->_invited.erase(oldNick);
	
	logMessage("Nick updated in channel ", GREEN, this->_name \
		+ ": " + oldNick + " -> " + newNick, BLUE);
	return true;
}
