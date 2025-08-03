/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 18:07:26 by caio              #+#    #+#             */
/*   Updated: 2025/08/03 15:33:15 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

#define BUFFER_SIZE 4096



int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Invalid arguments number!" << std::endl;
        return (-1);
    }
    
    std::string port = argv[1];
    std::string pass = argv[2];
       
    Server server(port, pass);
    
    int server_fd = server.createSocket();
    int bindStatus = server.bindSocket();
    int listenStatus = server.listenSocket();
    
    if (server_fd != -1 && bindStatus != -1 && listenStatus != -1)
        std::cout<< "Server running on port: " << port << std::endl;

    //POLL FD VECTOR
    
    std::vector<struct pollfd> client_fds;
    struct pollfd server_pollfd;
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    client_fds.push_back(server_pollfd);
    
    char buffer[BUFFER_SIZE];
    
    while(true)
    {
        int poll_count = poll(&client_fds[0], client_fds.size(), -1);

        if (poll_count == -1)
        {
            std::cerr << "Poll failed" << std::endl;
            break;
        }

        size_t i;
        for (i = 0; i < client_fds.size(); i++)
        {
            if(client_fds[i].revents & POLLIN)
            {
                if(client_fds[i].fd == server_fd) //NEW CONNETION
                {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                    
                    if (client_socket < 0)
                    {
                        std::cerr << "Accept failed!" << std::endl;
                        continue;
                    }
                    fcntl(client_socket, F_SETFL, O_NONBLOCK);
                    std::cout << "New client connected! FD= "<< client_socket << std::endl;
                    struct pollfd client_pollfd;
                    client_pollfd.fd = client_socket;
                    client_pollfd.events = POLLIN;
                    client_pollfd.revents = 0;
                    client_fds.push_back(client_pollfd);
                }
                else
                {
                    //EXISTING CLIENT SENT SOMETHING
                    memset(buffer, 0, BUFFER_SIZE);
                    int bytes_received = recv(client_fds[i].fd, buffer, BUFFER_SIZE, 0);
                    if (bytes_received <= 0)
                    {
                        std::cout << "Client disconnected!" << std::endl;
                        close(client_fds[i].fd);
                        client_fds.erase(client_fds.begin() + i);
                        --i; //INDEX CORRECTION AFTER ERASING
                    }
                    else
                    {
                        std::string msg(buffer, bytes_received);
                        std::cout << "Received from FD= " << client_fds[i].fd << ": " << msg;

                        //RESEND MESAGE TO CLIENT (FOR NOW ONLY FOR DEBUGGING PURPOSES)
                        msg += "\r\n"; //IRC FORMAT
                        send(client_fds[i].fd, msg.c_str(), msg.length(), 0);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
    
}
