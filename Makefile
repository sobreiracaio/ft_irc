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