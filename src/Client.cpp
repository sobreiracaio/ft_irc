#include "../include/Client.hpp"

Client::Client(int client_socket, sockaddr_in client_addr) \
	: _client_fd(client_socket), _client_addr(client_addr), \
	_isRegistered(false), _hasPassword(false), _hasNick(false), _hasUser(false)
{
	//Converts IP address to string in a secure way
	char ip_str[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, \
		INET_ADDRSTRLEN) != NULL)
		this->_hostname = std::string(ip_str);
	else
		this->_hostname = "unknown";
	
	logMessage("New client connected! FD= ", BLUE, itoa(client_socket), GREEN);
	this->_lastActivity = time(NULL);
	
}

Client::~Client()
{
	if(this->_client_fd > 0)
		close(this->_client_fd);
}

int Client::getFd() const
{
	return this->_client_fd;
}

std::string Client::getHostname() const
{
	return this->_hostname;
}

sockaddr_in Client::getClientAddr() const
{
	return this->_client_addr;
}

std::string Client::getNickname() const
{
	return this->_nickname;
}

std::string Client::getRealname() const
{
	return this->_realname;
}

std::string Client::getUsername() const
{
	return this->_username;
}

std::string Client::getPassword() const
{
	return this->_password;
}

std::string Client::getBuffer() const
{
	return this->_buffer;
}

bool Client::isRegistered() const
{
	return (this->_isRegistered);
}

void Client::appendBuffer(const std::string &data)
{
	// Prevent buffer overflow
	if (this->_buffer.length() + data.length() > MAX_BUFFER_SIZE)
	{
		logMessage("WARNING: ", YELLOW, \
			"Buffer overflow prevented for client FD=" \
			+ itoa(_client_fd), WHITE);
		this->_buffer.clear(); //Cleans up buffer to avoid errors
		return;
	}
	
	this->_buffer += data;
}

bool Client::isDataComplete() const
{
	return (this->_buffer.find("\r\n") != std::string::npos \
			|| this->_buffer.find("\n") != std::string::npos);
}

void Client::cleanBuffer()
{
	this->_buffer.clear();
}

std::string Client::getNextCompleteMessage()
{
	std::string message;
	size_t pos = this->_buffer.find("\r\n");

	if (pos == std::string::npos)
		pos = this->_buffer.find("\n");

	if (pos != std::string::npos)
	{
		message = this->_buffer.substr(0, pos);
		this->_buffer.erase(0, pos + (this->_buffer[pos] == '\r' ? 2 : 1));
	}

	return message;
}

void Client::checkRegistrationComplete()
{
	bool was_registered = this->_isRegistered;
	
	// Checking if user was created, if NICK and PASS were previously added
	if (this->_hasPassword && this->_hasNick && !this->_hasUser)
	{
		// Otherwise, create standard user with nickname based data
		this->_username = this->_nickname;
		this->_realname = this->_nickname;
		this->_hasUser = true;
		logMessage("Auto-generated USER data for: ", CYAN, \
			this->_nickname, WHITE);
	}
	
	// Client successfully registered after user creation
	this->_isRegistered = (this->_hasNick && this->_hasUser \
		&& this->_hasPassword);
	
	if (!was_registered && this->_isRegistered)
	{
		logMessage("Client registration data complete! Nick: ", GREEN, \
			this->_nickname + " User: " + this->_username + " Pass: " \
			+ this->_password, BLUE);
	}
	else if (!this->_isRegistered)
	{
		logMessage("Client registration incomplete - Nick: " \
				+ std::string(this->_hasNick ? "OK" : "MISSING") \
				+ " User: " + (this->_hasUser ? "OK" : "MISSING") \
				+ " Pass: " + (this->_hasPassword ? "OK" : "MISSING"), \
				YELLOW, "", WHITE);
	}
}

void Client::joinChannel(const std::string &channelName)
{
	this->_channels.insert(channelName);
}

void Client::leaveChannel(const std::string &channelName)
{
	this->_channels.erase(channelName);
}

bool Client::isInChannel(const std::string &channelName) const
{
	return (this->_channels.find(channelName) != this->_channels.end());
}

std::set<std::string> Client::getChannels() const
{
	return this->_channels;
}


bool Client::isValidInput(const std::string &input) const
{
	// Verifies if data inserted has harmful control caracters
	for (size_t i = 0; i < input.length(); i++)
	{
		unsigned char c = static_cast<unsigned char>(input[i]);
		
		// Allow only printable chars and some control chars
		if (c < 32 && c != '\r' && c != '\n' && c != '\t')
			return false;
			
		// NULL control char is not allowed
		if (c == 0)
			return false;
	}
	return true;
}

time_t Client::getLastActivity (void)
{
	return (this->_lastActivity);
}
