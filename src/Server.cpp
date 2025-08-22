/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: crocha-s <crocha-s@student.42.fr>          #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025-08-22 13:58:36 by crocha-s          #+#    #+#             */
/*   Updated: 2025-08-22 13:58:36 by crocha-s         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

Server::Server(int port, std::string password): \
		_port(port), _password(password), _server_fd(-1)
{
	this->_server_name = "ircserv";
}

Server::~Server()
{	
	if(this->_server_fd > 0)
		close(this->_server_fd);
}

int Server::_createSocket()
{
	int listen_socket = socket(AF_INET, SOCK_STREAM, 0);

	if(listen_socket == -1)
	{
		logMessage("ERROR: ", RED, \
			"Can't create socket!", YELLOW, ERR);
		return -1;
	}
	
	if (fcntl(listen_socket, F_SETFL, O_NONBLOCK) == -1)
	{
		logMessage("ERROR: ", RED, \
			"Failed to set server socket to non-blocking mode!", YELLOW, ERR);
		close(listen_socket);
		return -1;
	}

	this->_server_fd = listen_socket;

	int opt = 1;
	if (setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		logMessage("ERROR: ", RED, \
			"Failed to set socket options!", YELLOW, ERR);
		return (-1);
	}
	
	logMessage("Server socket created successfully!", BLUE, "", RESET);
	return (this->_server_fd);
}

int Server::_bindSocket(void)
{
	memset(&this->_server_addr, 0, sizeof(this->_server_addr));
	this->_server_addr.sin_family = AF_INET;
	this->_server_addr.sin_port = htons(this->_port);
	this->_server_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(this->_server_fd, (struct sockaddr*)&this->_server_addr, sizeof(this->_server_addr)) == -1)
	{
		logMessage("ERROR: ", RED, "Can't bind to IP/port!", YELLOW, ERR);
		return -1;
	}
	logMessage("Bind successful!", BLUE, "", RESET);
	return (0);
}

int Server::_listenSocket()
{
	if(listen(this->_server_fd, SOMAXCONN) == -1)
	{
		logMessage("ERROR: ", RED, "Can't listen!", YELLOW, ERR);
		return -1;
	}
	else
	{
		logMessage("Waiting for connections...", GREEN, "", RESET);
	}
	return (0);
}

bool Server::serverInit()
{
	if(this->_createSocket() == -1 ||
			this->_bindSocket() == -1 ||
			this->_listenSocket() == -1)
		return (false);
	
	// Poll setup for server socket
	struct pollfd server_pollfd;
	server_pollfd.fd = this->_server_fd;
	server_pollfd.events = POLLIN;
	server_pollfd.revents = 0;
	this->_poll_fds.push_back(server_pollfd);

	return (true);
}

void Server::run()
{
	while(true)
	{
		time_t now = time(NULL); // Time in seconds
		int poll_count = poll(&this->_poll_fds[0], \
							this->_poll_fds.size(), 5000);
		if (poll_count == -1)
		{
			logMessage("ERROR: ", RED, "Poll failed!", YELLOW, ERR);
			break;
		}
		if(poll_count == 0 && this->_poll_fds.size() > 1)
		{
			for(size_t i = 1; i < this->_poll_fds.size(); i++)
			{
				Client *client = this->getClient(this->_poll_fds[i].fd);
				
				// 5 minutes idle is automatically disconected
				if(now - client->getLastActivity() > 300)
					this->quitServer("QUIT", client->getFd(), "Idle");
			}
		}
		size_t poll_size = this->_poll_fds.size();
		for (size_t i = 0; i < poll_size; i++)
		{
			if (i >= this->_poll_fds.size())
				break;
			if(this->_poll_fds[i].revents & POLLIN)
			{
				if(this->_poll_fds[i].fd == this->_server_fd)
				{
					this->_acceptNewClient();
				}
				else
				{
					this->_handleClientData(this->_poll_fds[i].fd);
					if(i >= this->_poll_fds.size())
						break;
				}
			}
			else if(this->_poll_fds[i].revents & (POLLHUP | POLLERR))
			{
				//Client disconnected by socket error
				if(this->_poll_fds[i].fd != this->_server_fd)
				{
					this->_removeClient(this->_poll_fds[i].fd);
					poll_size = this->_poll_fds.size(); //Update size;
					i--; // Adjust index to avoid jumping over elements
				}
			}
		}
	}
}

int Server::getServerFd(void)
{
	return this->_server_fd;
}

std::string Server::getServerName(void) const
{
	return (this->_server_name);
}

void Server::_sendErrorReply(int client_fd, int code, const std::string &message)
{
	Client *client = getClient(client_fd);
	if (!client)
		return;

	std::ostringstream oss;
	oss << ":" << _server_name << " " << std::setfill('0') << std::setw(3) << code << " " << (client->getNickname().empty() ? "*" : client->getNickname()) << " :" << message << "\r\n";

	std::string reply = oss.str();
	if (send(client_fd, reply.c_str(), reply.length(), 0) == -1)
		logMessage("ERROR: ", RED, "Failed to send error reply!", YELLOW, ERR);
}

std::vector<std::string> Server::_splitMessage(const std::string &message)
{
	std::vector<std::string> result;
	std::istringstream iss(message);
	std::string token;

	while (iss >> token)
		result.push_back(token);
	return result;
}

void Server::_welcomeMessage(Client* client)
{
	std::string user = "" + client->getNickname() + "";
	std::stringstream welcome;

	// Total frame width without the '#'
	const int totalWidth = 70;

	// Calculates spaces to centralize username
	int padding = totalWidth - static_cast<int>(user.size());
	if (padding < 0) 
		padding = 0;
	int padLeft = padding / 2;
	int padRight = padding - padLeft;

	welcome << "########################################################################\n";
	welcome << "#                         Bem-vindo ao IRC Server                      #\n";
	welcome << "#" << std::string(padLeft, ' ') << user << std::string(padRight, ' ') << "#\n";
	welcome << "#  Comandos disponíveis:                                               #\n";
	welcome << "#                                                                      #\n";
	welcome << "#  /JOIN   #canal [senha]       -> Entrar em um canal                  #\n";
	welcome << "#  /PRIVMSG alvo :mensagem      -> Enviar mensagem privada             #\n";
	welcome << "#  /NOTICE alvo mensagem       -> Enviar notificação                   #\n";
	welcome << "#           OBS: alvo pode ser user ou #canal                          #\n";
	welcome << "#  /NICK   novonick             -> Alterar seu apelido                 #\n";
	welcome << "#  /QUIT                        -> Sair do servidor                    #\n";
	welcome << "#                                                                      #\n";
	welcome << "#  Comandos de canal:                                                  #\n";
	welcome << "#  /KICK   #canal nick [motivo] -> Expulsar usuário                    #\n";
	welcome << "#  /INVITE nick #canal          -> Convidar usuário                    #\n";
	welcome << "#  /TOPIC  #canal [tópico]      -> Definir/ver tópico                  #\n";
	welcome << "#  /MODE   #canal +i/-i         -> Modo convidado obrigatório/livre    #\n";
	welcome << "#  /MODE   #canal +t/-t         -> Apenas operadores mudam tópico      #\n";
	welcome << "#  /MODE   #canal +k/-k senha   -> Definir/remover senha               #\n";
	welcome << "#  /MODE   #canal +o/-o nick    -> Dar/remover operador                #\n";
	welcome << "#  /MODE   #canal +l/-l [n]     -> Definir/remover limite              #\n";
	welcome << "########################################################################\n";

	std::string line;

	while(getline(welcome, line, '\n'))
	{
		std::string msg = ":" + client->getHostname() \
		+ " 001 " + client->getNickname() + " :" + line + "\r\n";
		send(client->getFd(), msg.c_str(), msg.length(), 0);
	}
}

void Server::cleanUp()
{
	// Free all clients
	for (std::map<int, Client*>::iterator it = this->_clients.begin(); \
		it != this->_clients.end(); ++it) {
		delete it->second; //Deletes the second on the ::map, which is Client*
	}
	this->_clients.clear();

	//Free all channels
	for (std::map<std::string, Channel*>::iterator it = \
		this->_channels.begin(); it != this->_channels.end(); ++it) {
		delete it->second;
	}
	this->_channels.clear();

	std::vector<pollfd>().swap(this->_poll_fds);
	// std::vector<pollfd>() creates a temporary, empty vector and .swap(this->_poll_fds) swaps the contents of the empty vector with this->_poll_fds. The old data is now in the temporary vector, which is immediately destroyed, freeing its memory.
}
