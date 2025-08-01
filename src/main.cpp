/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 18:07:26 by caio              #+#    #+#             */
/*   Updated: 2025/07/31 18:14:26 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"



int main()
{
    Server server("6667", "123");
    
    int fd = server.createSocket();
    server.bindSocket();
    server.listenSocket();
      
    //Accept a call
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    int clientSocket = accept(fd, (sockaddr *)&client, &clientSize);

    if (clientSocket == -1)
    {
        std::cerr << "Problem with client connecting!" << std::endl;
        return -4;
    }

    //Close listening socket

    close(fd);
    memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);

    int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0);

    if (result)
    {
        std::cout << host << " connected on " << service << std::endl;
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connected on " << ntohs(client.sin_port) << std::endl;
    }
    
    // While receiving - display message, echo message
    char buf[4096];
   
    while (true)
    {
        //Clear buffer
        memset(buf, 0, 4096);
        //Wait for a message
        int bytesReceived = recv(clientSocket, buf, 4096, 0);
        if (bytesReceived == -1)
        {
            std::cerr << "There was a connection issue" << std::endl;
            break;
        }
        
        if (bytesReceived == 0)
        {
            std::cout << "The client disconnected" << std::endl;
            break;
        }
        std::string msg = std::string(buf, 0, bytesReceived); 
        //Display message
        std::cout << "Received: " << msg << std::endl;
        //Resend message
        std::string resend_msg = "Resend: " + msg;
        send(clientSocket, resend_msg.c_str(), resend_msg.length(), 0);
        
    }
    close(clientSocket);
    return 0;
    
}
