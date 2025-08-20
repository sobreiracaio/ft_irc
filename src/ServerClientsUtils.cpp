#include "../include/Server.hpp"

Client *Server::getClient(int client_fd)
{
	 std::map<int, Client*>::iterator it = this->_clients.find(client_fd); 
	
	// Verificação mais robusta: existe no mapa E o ponteiro não é nulo
	if (it != this->_clients.end() && it->second != NULL)
		return it->second;
	else
		return NULL;
}

Client *Server::getClientByNick(std::string const &nick)
{
	std::string formattedNick = nick;
	
	// Remove spaces and other chars
	size_t pos = formattedNick.find_first_of(" \r\n");
	if (pos != std::string::npos)
		formattedNick.erase(pos);

	std::map<int, Client*>::iterator client_it;
	for (client_it = this->_clients.begin(); client_it != this->_clients.end(); client_it++)
	{
		Client *temp = client_it->second;
		if(temp->getNickname() == formattedNick)
			return temp;
	}
	return NULL;
}

void Server::changeNick(std::string const &data, int client_fd)
{
	std::vector<std::string> tokens = _splitMessage(data);
	
	if (tokens.size() < 2)
	{
		this->_sendErrorReply(client_fd, ERR_NONICKNAMEGIVEN, "No nickname given");
		return;
	}
	
	Client *client = this->_clients[client_fd];
	if (!client)
		return;
	
	std::string old_nick = client->getNickname();
	std::string new_nickname = this->_checkDoubles(tokens[1], client_fd);
	
	// Se o nick não mudou (incluindo casos onde foi adicionado _), ainda processa
	if (old_nick == new_nickname && tokens[1] == new_nickname)
		return; // Mesmo nick, sem mudança necessária
	
	// Prepara a mensagem NICK
	std::string nick_msg = ":" + old_nick + "!" + client->getUsername() + "@" + 
						  client->getHostname() + " NICK :" + new_nickname + "\r\n";
	
	// Coleta todos os clientes que precisam ser notificados
	std::set<int> clients_to_notify;
	std::set<std::string> client_channels = client->getChannels();
	
	// Para cada canal onde o usuário está
	for (std::set<std::string>::const_iterator it = client_channels.begin(); 
		 it != client_channels.end(); ++it)
	{
		std::string channel_name = *it;
		if (channel_name[0] == '#')
			channel_name = channel_name.substr(1);
		
		Channel *channel = getChannelByName(channel_name);
		if (channel)
		{
			// Atualiza o nick no canal usando o novo método
			channel->updateUserNick(old_nick, new_nickname);
			
			// Coleta usuários do canal para notificar
			std::vector<std::string> users = channel->getUserList();
			for (size_t i = 0; i < users.size(); i++)
			{
				Client *user = getClientByNick(users[i]);
				if (user && user->getFd() != client_fd)
				{
					clients_to_notify.insert(user->getFd());
				}
			}
		}
	}
	
	// Atualiza o nickname do cliente
	client->setNickname(new_nickname);
	
	// Envia confirmação para o próprio cliente
	if (send(client_fd, nick_msg.c_str(), nick_msg.length(), 0) == -1)
		logMessage("ERROR: ", RED, "Failed to send NICK response!", YELLOW, ERR);
	
	// Notifica todos os clientes coletados
	for (std::set<int>::iterator it = clients_to_notify.begin(); 
		 it != clients_to_notify.end(); ++it)
	{
		if (send(*it, nick_msg.c_str(), nick_msg.length(), 0) == -1)
			logMessage("ERROR: ", RED, "Failed to broadcast NICK change", YELLOW, ERR);
	}
	
	logMessage("Nickname changed: ", GREEN, old_nick + " -> " + new_nickname, BLUE);
}

void Server::quitServer(std::string const &data, int client_fd, std::string msg)
{
	Client *client = getClient(client_fd);
	if (!client)
		return;
	
	size_t colon_pos = data.find(':');
	if (colon_pos != std::string::npos)
		msg = data.substr(colon_pos + 1);
	
	// Gets channel list before removing client
	std::set<std::string> client_channels = client->getChannels();
	std::string client_nick = client->getNickname();
	std::string client_user = client->getUsername();
	std::string client_host = client->getHostname();
	
	// Notify channels about the quitting
	for (std::set<std::string>::const_iterator it = client_channels.begin(); 
		 it != client_channels.end(); ++it)
	{
		std::string channel_name = *it;
		if (channel_name[0] == '#')
			channel_name = channel_name.substr(1);
			
		Channel *channel = getChannelByName(channel_name);
		if (channel)
		{
			// Remove user from channel
			channel->removeUser(client_nick);
			
			// Anuncia QUIT para outros usuários do canal
			channel->announceQuit(this, client, msg);
			
			// Remove channel if it is empty
			if (channel->isEmpty())
			{
				delete channel;
				_channels.erase(channel_name);
				logMessage("Empty channel removed: ", YELLOW, channel_name, RED);
			}
		}
	}
	
	logMessage("Client quit! Nick: ", YELLOW, client_nick, GREEN);
	
	// Removes client
	this->_removeClient(client_fd);
}
