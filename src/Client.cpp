/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: caio <caio@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 14:16:32 by caio              #+#    #+#             */
/*   Updated: 2025/08/04 16:23:54 by caio             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"

Client::Client(int client_socket, sockaddr_in client_addr):_client_fd(client_socket), _client_addr(client_addr)
{
    
}

Client::~Client(){}