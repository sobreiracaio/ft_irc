/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: crocha-s <crocha-s@student.42.fr>          #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025-08-22 13:58:25 by crocha-s          #+#    #+#             */
/*   Updated: 2025-08-22 13:58:25 by crocha-s         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

Server *g_server = NULL;

void signalHandler(int signal_type)
{
	switch (signal_type)
	{
	case SIGINT: // CTRL + C
		logMessage("\nReceived SIGINT (Ctrl+C). ", RED, \
			"Shutting down server gracefully...", YELLOW);
		if(g_server)
			g_server->cleanUp();
		logMessage("Server shutdown complete.", GREEN, "", WHITE);
		exit(0);
		break;

	case SIGTSTP: // CTRL + Z
		logMessage("\nReceived SIGTSTP (Ctrl+Z). ", RED, \
			"Server suspending... Warning - It won't be possible to recover this connection - use Ctrl+C to quit.", YELLOW);
		if(g_server)
			g_server->cleanUp();
		break;

	case SIGTERM: // System shutdown
		logMessage("\nReceived SIGTERM. ", RED, \
			"Shutting down server...", YELLOW);
		if (g_server)
		{
			delete g_server;
			g_server = NULL;
		}
		exit(0);
		break;

	case SIGPIPE: // Broken pipe
		logMessage("\nReceived SIGPIPE. ", RED, \
			"Broken pipe detected.", YELLOW);
		break;
	
	default:
		logMessage("\nReceived unknown signal: ", RED, \
			itoa(signal_type), YELLOW);
		break;
	}
}

bool checkPort(std::string &port)
{
	if(!isNum(port))
	{
		logMessage("ERROR: ", RED, \
			"Port must be a numerical value!", YELLOW, ERR);
		return (false);
	}
	
	int int_port = atoi(port.c_str());
	
	if(!isValidPort(int_port))
	{
		logMessage("ERROR: ", RED, \
			"Invalid port number!", YELLOW, ERR);
		return (false);
	}
	return (true);
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		logMessage("Invalid number of arguments! ", RED, \
			"Try ./ircserv <port> <password>", YELLOW, ERR);
		return (-1);
	}

	std::string port = argv[1];
	std::string pass = argv[2];
	
	int int_port = 0;
	
	if(!checkPort(port))
		return (-1);

	int_port = atoi(port.c_str());
	
	signal(SIGINT, signalHandler);
	signal(SIGTSTP, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGPIPE, SIG_IGN);

	Server server(int_port, pass);
	g_server = &server;

	if (server.serverInit())
		logMessage("Server running on port: ", BLUE, port, GREEN);
	else
		return (-1);

	server.run();
	g_server = NULL;
	return 0;
}
