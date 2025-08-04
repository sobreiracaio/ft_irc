/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 18:07:26 by caio              #+#    #+#             */
/*   Updated: 2025/08/04 14:44:02 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"





int main(int argc, char **argv)
{
    if (argc != 3)
    {
        logMessage("Invalid number of arguments", RED, "Try ./ircserv <port> <password>", YELLOW, ERR);
        //std::cerr << "Invalid arguments number!" << std::endl;
        return (-1);
    }
    
    std::string port = argv[1];
    std::string pass = argv[2];
       
    Server server(port, pass);
    
    if (server.serverInit())
        logMessage("Server running on port: ", BLUE, port, GREEN);
    else
    {
        return (-1);
    }
    int server_fd = server.getServerFd();

    //POLL FD VECTOR

    server.run();
    
 
    close(server_fd);
    return 0;
    
}
