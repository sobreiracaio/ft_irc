#include "../include/Server.hpp"

void Server::_acceptNewClient(void)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_socket = accept(this->_server_fd, (struct sockaddr *)&client_addr, &client_len);
					
	if (client_socket < 0)
	{
		logMessage("ERROR: ", RED, "Accept failed!", YELLOW, ERR);
		return;
	}
	
	if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1)
	{
		logMessage("ERROR: ", RED, "Failed to set client socket non-blocking!", YELLOW, ERR);
		close(client_socket);
		return;
	}
	
	//Create new client and add it to the map according to its fd 
	Client* new_client = new Client(client_socket, client_addr);
	this->_clients[client_socket] = new_client;
	
	//Poll setup for client socket
	struct pollfd client_pollfd;
	client_pollfd.fd = client_socket;
	client_pollfd.events = POLLIN;
	client_pollfd.revents = 0;
	this->_poll_fds.push_back(client_pollfd);
}


void Server::_handleClientData(int client_fd)
{
	char buffer[BUFFER_SIZE];
	Client *client = getClient(client_fd);
	
	if (!client)
		return;
	
	memset(buffer, 0, BUFFER_SIZE);
	int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
	
	if (bytes_received <= 0)
	{
		if (bytes_received == 0)
			this->_removeClient(client_fd);
		return;        
	}
	   
	buffer[bytes_received] = '\0';
	
	// // Clean problematic control characters from buffer
	std::string data;
	for (int i = 0; i < bytes_received; i++)
	{
		unsigned char c = static_cast<unsigned char>(buffer[i]);
		
		// Filter control characters, but keep \r\n
		if (c >= 32 || c == '\r' || c == '\n' || c == '\t')
		{
			data += c;
		}
		else if (c == 4) // Ctrl+D (EOF)
		{
			// Ignores Ctrl+D but continues processing
			logMessage("Ctrl+D received from FD ", YELLOW, \
				 itoa(client_fd), WHITE);
			continue;
		}
	}
	
	if (data.empty())
		return;
	
	if(data.find("PONG", 0, 4) == std::string::npos)
		logMessage("from FD = " + itoa(client_fd) + ":\n", BLUE, data, WHITE);
	
	// SEMPRE adicionar dados ao buffer primeiro
	client->appendBuffer(data);
	
	// Se cliente não está registrado, processa registro
	if(!client->isRegistered())
	{
		bool processed_any_command = false;
		
		// Processa todos os comandos de registro disponíveis no buffer
		while(client->isDataComplete())
		{
			std::string message = client->getNextCompleteMessage();
			
			if (message.empty())
				break;
			
			logMessage("Processing registration command: ", CYAN, message, WHITE);
			processed_any_command = true;
			
			// Processa comandos de registro
			if (message.length() >= 5)
			{
				std::string cmd = message.substr(0, 5);
				
				if(cmd == "PASS ")
				{
					client->parsePassCommand(message);
				}
				else if(cmd == "NICK ")
				{
					client->parseNickCommand(message);
				}
				else if(cmd == "USER ")
				{
					client->parseUserCommand(message);
				}
			}
			// CORREÇÃO: Para nc, também aceita comandos sem espaço extra
			else if (message.length() >= 4)
			{
				std::string cmd = message.substr(0, 4);
				if(cmd == "NICK")
				{
					// Processa NICK mesmo sem espaço (para nc)
					if(message.length() > 4)
						client->parseNickCommand("NICK " + message.substr(4));
				}
				else if(cmd == "PASS")
				{
					if(message.length() > 4)
						client->parsePassCommand("PASS " + message.substr(4));
				}
				else if(cmd == "USER")
				{
					if(message.length() > 4)
						client->parseUserCommand("USER " + message.substr(4));
				}
			}
		}
		
		// NOVO: Se processamos comandos e temos PASS + NICK, tenta completar registro
		if(processed_any_command && !client->getPassword().empty() && !client->getNickname().empty())
		{
			// Força verificação de registro (que auto-gerará USER se necessário)
			client->checkRegistrationComplete();
		}
		
		// Só verifica se está totalmente registrado após processar todos os comandos
		if (client->isRegistered())
		{
			// Verifica se a senha está correta APENAS quando totalmente registrado
			if(!this->_checkPassword(client->getPassword()))
			{
				logMessage("Password check failed for client: ", RED, client->getNickname(), YELLOW);
				this->_sendErrorReply(client_fd, ERR_PASSWDMISMATCH, "Password incorrect!");
				// Aguarda um pouco antes de desconectar para evitar "connection reset"
				usleep(100000); // 100ms
				this->_removeClient(client_fd);
				return;
			}
			
			// Verifica duplicatas de nickname
			std::string nick = this->_checkDoubles(client->getNickname(), client_fd);
			client->setNickname(nick);
			this->_welcomeMessage(client);
			logMessage("Client fully registered: ", GREEN, client->getNickname(), BLUE);
		}
		return;
	}

		
	// Client is registered, process commands normally
	while(client->isDataComplete())
	{
		std::string message = client->getNextCompleteMessage();
		
		if (message.empty())
			break;
			
		int command_code = this->parseCommand(message);
		
		// Execute command and check if the client was removed
		bool client_still_exists = this->executeCommand(client_fd, command_code, message);
		
		if (!client_still_exists)
		{
			return; // Cliente was removed, does not try to access its data anymore
		}
		
		// Checks if client still exists after command execution
		client = getClient(client_fd);
		if (!client)
			return;
	}
}

void Server::_removeClient(int client_fd)
{
	// Remove from poll fds first
	std::vector<struct pollfd>::iterator poll_it;
	for (poll_it = this->_poll_fds.begin(); poll_it != this->_poll_fds.end(); poll_it++)
	{
		if(poll_it->fd == client_fd)
		{
			this->_poll_fds.erase(poll_it);
			break;
		}   
	}
	
	// Find and remove client
	std::map<int, Client*>::iterator client_it = this->_clients.find(client_fd);
	if (client_it != this->_clients.end())
	{
		Client *client = client_it->second;
		
		// Remove client from all channels before deleting
		if (client)
		{
			std::set<std::string> client_channels = client->getChannels();
			std::string client_nick = client->getNickname();
			
			for (std::set<std::string>::const_iterator it = client_channels.begin(); 
				 it != client_channels.end(); ++it)
			{
				std::string channel_name = *it;
				if (channel_name[0] == '#')
					channel_name = channel_name.substr(1);
					
				Channel *channel = getChannelByName(channel_name);
				if (channel)
				{
					channel->removeUser(client_nick);
					
					// Remove empty channels
					if (channel->isEmpty())
					{
						delete channel;
						_channels.erase(channel_name);
						logMessage("Empty channel removed: ", YELLOW, channel_name, RED);
					}
				}
			}
		}
		
		// Now safe to delete client
		delete client;
		this->_clients.erase(client_it);
	}

	close(client_fd);
	logMessage("Client disconnected! FD = ", RED, itoa(client_fd), YELLOW);
}
