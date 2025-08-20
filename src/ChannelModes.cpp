#include "../include/Channel.hpp"

// Modes management
bool Channel::setMode(char mode, bool enable, std::string const &param)
{
	if (enable)
	{
		this->_modes.insert(mode);
		if (mode == MODE_KEY)
			this->_key = param;
		else if (mode == MODE_LIMIT && !param.empty())
			this->_user_limit = atoi(param.c_str());
	}
	else
	{
		this->_modes.erase(mode);
		if (mode == MODE_KEY)
			this->_key.clear();
		else if (mode == MODE_LIMIT)
			this->_user_limit = 0;
	}

	logMessage("Mode change in ", BLUE,this->_name + ": " \
				+ (enable ? "+" : "-") + mode, GREEN);
	return true;
}

bool Channel::hasMode(char mode) const
{
	return this->_hasMode(mode);
}

std::string Channel::getModeString() const
{
	std::string modes = "+";
	for (std::set<char>::const_iterator it = this->_modes.begin(); \
		it != this->_modes.end(); ++it)
		modes += *it;

	if (hasMode(MODE_KEY) && !_key.empty())
		modes += " " + this->_key;
	if (hasMode(MODE_LIMIT) &&this->_user_limit > 0)
		modes += " " + itoa(_user_limit);

	return modes;
}

// Operators management
bool Channel::addOp(std::string const &user)
{
	if (!hasUser(user))
		return false;

	this->_operators.insert(user);
	logMessage("User became operator in ", GREEN,this->_name + ": " \
		+ user, BLUE);
	return true;
}

bool Channel::removeOp(std::string const &user)
{
	if (this->_operators.find(user) == this->_operators.end())
		return false;

	this->_operators.erase(user);
	logMessage("User lost operator in ", YELLOW,this->_name + ": " \
			+ user, BLUE);
	return true;
}

bool Channel::isOp(std::string const &user) const
{
	return (this->_operators.find(user) != this->_operators.end());
}

