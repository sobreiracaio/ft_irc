/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 18:07:26 by caio              #+#    #+#             */
/*   Updated: 2025/08/03 21:33:13 by caio             ###   ########.fr       */
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
    
    int server_fd = server.getServerFd();

    //POLL FD VECTOR

    server.run();
    
    // std::vector<struct pollfd> fds;
    // struct pollfd server_pollfd;
    // server_pollfd.fd = server_fd;
    // server_pollfd.events = POLLIN;
    // server_pollfd.revents = 0;
    // fds.push_back(server_pollfd);
    
    // char buffer[BUFFER_SIZE];
    
    // while(true)
    // {
    //     int poll_count = poll(&fds[0], fds.size(), -1);

    //     if (poll_count == -1)
    //     {
    //         std::cerr << "Poll failed" << std::endl;
    //         break;
    //     }

    //     size_t i;
    //     for (i = 0; i < fds.size(); i++)
    //     {
    //         if(fds[i].revents & POLLIN)
    //         {
    //             if(fds[i].fd == server_fd) //NEW CONNETION
    //             {
    //                 struct sockaddr_in client_addr;
    //                 socklen_t client_len = sizeof(client_addr);
    //                 int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                    
    //                 if (client_socket < 0)
    //                 {
    //                     std::cerr << "Accept failed!" << std::endl;
    //                     continue;
    //                 }
    //                 fcntl(client_socket, F_SETFL, O_NONBLOCK);
    //                 std::cout << "New client connected! FD= "<< client_socket << std::endl;
    //                 struct pollfd client_pollfd;
    //                 client_pollfd.fd = client_socket;
    //                 client_pollfd.events = POLLIN;
    //                 client_pollfd.revents = 0;
    //                 fds.push_back(client_pollfd);
    //             }
    //             else
    //             {
    //                 //EXISTING CLIENT SENT SOMETHING
    //                 memset(buffer, 0, BUFFER_SIZE);
    //                 int bytes_received = recv(fds[i].fd, buffer, BUFFER_SIZE, 0);
    //                 if (bytes_received <= 0)
    //                 {
    //                     std::cout << "Client disconnected!" << std::endl;
    //                     close(fds[i].fd);
    //                     fds.erase(fds.begin() + i);
    //                     --i; //INDEX CORRECTION AFTER ERASING
    //                 }
    //                 else
    //                 {
    //                     std::string msg(buffer, bytes_received);
    //                     std::cout << "Received from FD= " << fds[i].fd << ": " << msg;

    //                     //RESEND MESAGE TO CLIENT (FOR NOW ONLY FOR DEBUGGING PURPOSES)
    //                     msg += "\r\n"; //IRC FORMAT
    //                     std::string msg2 = "Resend: " + msg + "\r\n";
    //                     send(fds[i].fd, msg2.c_str(), msg.length(), 0);
    //                 }
    //             }
    //         }
    //     }
    // }

    close(server_fd);
    return 0;
    
}
