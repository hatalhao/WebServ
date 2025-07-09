NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3

CXXFLAGS += -MMD -MP

CXXFLAGS += -fsanitize=address

MAKEFLAGS = -j --no-print-directory 

# MAKEFLAGS += -silent

SRCS = $(wildcard src/*.cpp)

OBJS = $(SRCS:.cpp=.o)

DEPS = $(SRCS:.cpp=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re:
	make fclean
	make all

.PHONY: all clean fclean re

.SECONDARY: $(OBJS)

-include $(DEPS)