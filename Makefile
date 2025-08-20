#COLORS
GREEN	:= \033[0;32m
MAG		:= \033[0;35m
CYAN	:= \033[0;36m
RST		:= \033[0m

NAME			=	ircserv

COMPILE			= 	c++

FLAGS 			=	-Wall -Wextra -Werror -std=c++98

EXTRA_FLAGS		=	-pedantic-errors -g

INCLUDE			=	-Iinclude

SRC_DIR			=	src
INC_DIR			=	includes
BIN_DIR			=	bin
SRCS			=	$(SRC_DIR)/main.cpp \
					$(SRC_DIR)/Client.cpp \
					$(SRC_DIR)/Server.cpp \
					$(SRC_DIR)/ServerClients.cpp \
					$(SRC_DIR)/ServerClientsUtils.cpp \
					$(SRC_DIR)/ServerValidate.cpp \
					$(SRC_DIR)/ServerChannels.cpp \
					$(SRC_DIR)/ServerCommand.cpp \
					$(SRC_DIR)/ServerModeration.cpp \
					$(SRC_DIR)/Utils.cpp \
					$(SRC_DIR)/Channel.cpp


OBJS	:= $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.o,$(SRCS))

all: $(BIN_DIR) $(NAME)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(NAME): $(OBJS)
	@echo "Linking $(NAME)..."
	@$(COMPILE) $(FLAGS) $(EXTRA_FLAGS) -o $@ $^
	@echo "\n$(CYAN)----- $(MAG)❤ $(GREEN)$(NAME) compiled! $(MAG)❤ $(CYAN)-----$(RST)\n"

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	@echo "Compiling $<..."
	@$(COMPILE) $(FLAGS) $(EXTRA_FLAGS) -I $(INC_DIR) -c $< -o $@

clean:
	@echo "Cleaning objects..."
	@rm -rf $(BIN_DIR)

fclean: clean
	@echo "Cleaning executable..."
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re