# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: caio <caio@student.42.fr>                  +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/08/03 13:35:25 by caio              #+#    #+#              #
#    Updated: 2025/08/05 19:02:15 by caio             ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME			=	ircserv

COMPILE			= 	c++

FLAGS 			=	-Wall -Wextra -Werror -std=c++98

EXTRA_FLAGS		=	-pedantic-errors -g

INCLUDE			=	-Iinclude

SRCS				=	src/main.cpp src/Client.cpp src/Server.cpp src/Utils.cpp src/Channel.cpp

OBJS			=	$(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(COMPILE) $(FLAGS) $(EXTRA_FLAGS) $(SRCS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all