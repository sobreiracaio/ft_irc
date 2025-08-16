/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 18:07:26 by caio              #+#    #+#             */
/*   Updated: 2025/08/16 15:43:15 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

Server *g_server = NULL;

void signalHandler(int signal_type)
{
    switch (signal_type)
    {
    case SIGINT:  // CTRL + C
        logMessage("\nReceived SIGINT (Ctrl+C). ", RED, "Shutting down server gracefully...", YELLOW);
        if(g_server)
            g_server->cleanUp();
        logMessage("Server shutdown complete.", GREEN, "", WHITE);
        exit(0);
        break;
    
    case SIGTSTP: // CTRL + Z
        logMessage("\nReceived SIGTSTP (Ctrl+Z). ", RED, "Signal ignored - use Ctrl+C to quit.", YELLOW);
        return;
        break;

    case SIGTERM: // System shutdown
        logMessage("\nReceived SIGTERM. ", RED, "Shutting down server...", YELLOW);
        if (g_server)
        {
            delete g_server;
            g_server = NULL;
        }
        exit(0);
        break;
        
    case SIGPIPE: // Broken pipe
        logMessage("\nReceived SIGPIPE. ", RED, "Broken pipe detected.", YELLOW);
        break;
    
    default:
        logMessage("\nReceived unknown signal: ", RED, itoa(signal_type), YELLOW);
        break;
    }
}



int main(int argc, char **argv)
{
    
    if (argc != 3)
    {
        logMessage("Invalid number of arguments! ", RED, "Try ./ircserv <port> <password>", YELLOW, ERR);
        return (-1);
    }
    
    std::string port = argv[1];
    std::string pass = argv[2];
    
    int int_port = 0;
    
    if(isNum(port))
    {
        int_port = atoi(port.c_str());
    }
    else
    {
        logMessage("ERROR: ", RED, "Invalid port number!", YELLOW, ERR);
        return (-1);
    }

    signal(SIGINT, signalHandler);
    signal(SIGTSTP, SIG_IGN);
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
