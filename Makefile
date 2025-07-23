ERASE		:=  \033[2K\r
BLUE		:=	\033[34m
YELLOW		:=	\033[33m
GREEN		:=	\033[32m
MAGENTA		:=	\033[35m
CYAN		:=	\033[36m
BOLD		:=	\033[1m
END			:=	\033[0m

NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Werror -Wextra -std=c++98 -g3

CXXFLAGS += -MMD -MP

CXXFLAGS += -fsanitize=address

MAKEFLAGS = -j --no-print-directory


OBJS_DIR = tmp/objs
DEPS_DIR = tmp/deps

SRCS =  $(shell find src -name "*.cpp")

OBJS = $(patsubst src/%.cpp, $(OBJS_DIR)/%.o, $(SRCS))
DEPS = $(patsubst src/%.cpp, $(DEPS_DIR)/%.d, $(SRCS))

all : $(NAME)

$(NAME) : $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $@
	@printf "$(ERASE)$(GREEN)--> $@ made <--$(END)\n"

$(OBJS_DIR)/%.o: src/%.cpp Makefile
	@mkdir -p $(dir $@) $(dir $(DEPS_DIR)/$*.d)
	@$(CXX) $(CXXFLAGS) -MF $(DEPS_DIR)/$*.d -c $< -o $@
	@printf "$(BLUE) > Compilation :$(END) $<\r"

-include $(DEPS)

clean :
	@rm -rdf tmp
	@printf "$(MAGENTA)-->	tmp directory removed$(END)\n"

fclean : clean
	@rm -f $(NAME)
	@printf "$(MAGENTA)-->	$(NAME) removed$(END)\n"

re :
	@make fclean
	@make all

.PHONY: all clean fclean re
.SECONDARY:	$(OBJS)