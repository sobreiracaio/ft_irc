/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 18:07:26 by caio              #+#    #+#             */
/*   Updated: 2025/08/04 18:21:27 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"





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
       
    Server server(int_port, pass);
    
    if (server.serverInit())
        logMessage("Server running on port: ", BLUE, port, GREEN);
    else
        return (-1);
    
    int server_fd = server.getServerFd();

    server.run();
   
    close(server_fd);
    return 0;
    
}
