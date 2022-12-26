NAME		=	proxy_server

CC			=	c++ -std=c++17 -O2 -g
CFLAGS		=	-Wall -Werror -Wextra

HEADER_DIR	=	./includes
SRC_DIR		=	./src
OBJ_DIR		=	./obj

HEADER		=	logger.hpp \
				proxy_server.hpp \
				user.hpp \

SRC			=	main.cc \
				proxy_server.cc

OBJ			=	$(addprefix $(OBJ_DIR)/, $(SRC:.cc=.o))

RM_DIR		=	rm -rf
RM_FILE		=	rm -f

#COLORS
# C_NO		=	"\033[00m"
# C_OK		=	"\033[32m"
# C_GOOD		=	"\033[32m"

SUCCESS		=	$(C_GOOD)SUCCESS$(C_NO)
OK			=	$(C_OK)OK$(C_NO)

all			:	$(NAME)

$(OBJ)		: 	| $(OBJ_DIR)	

$(OBJ_DIR)	:
				@mkdir -p $(OBJ_DIR)
			
$(OBJ_DIR)/%.o	:	$(SRC_DIR)/%.cc ${HEADER_DIR}/*.hpp Makefile
					$(CC) $(CFLAG) -c $< -o $@

$(NAME)		:	$(OBJ)
				$(CC) $(CFLAGS) $(OBJ) -o $(NAME)
				@echo "Compiling..." [ $(NAME) ] $(SUCCESS)

clean		:
				@$(RM_DIR) $(OBJ_DIR)
				@echo "Cleaning..." [ $(OBJ_DIR) ] $(OK)

fclean		:	clean
				@$(RM_FILE) $(NAME)
				@echo "Deleting..." [ $(NAME) ] $(OK)

re			:	fclean all

.PHONY		:	all, clean, fclean, re, test