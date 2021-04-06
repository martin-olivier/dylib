SRC			=	tests.cpp

DYNLIBSRC	=	myDynLib.cpp

OBJ 		= 	$(SRC:.cpp=.o)

DYNLIBOBJ 	= 	$(DYNLIBSRC:.cpp=.o)

NAME 		= 	tests_run

DYNLIBNAME 	= 	myDynLib.so

CXXFLAGS 	= 	-std=c++11

CXX 		= 	g++

LDLIBS 		= 	-ldl -lgtest -lpthread -lgcov

all :
	g++ -std=c++11 -fPIC -shared myDynLib.cpp -o myDynLib.so
	g++ -std=c++11 $(SRC) -o $(NAME) -ldl -lgtest -lpthread -lgcov --coverage
	./$(NAME)
	gcovr -e $(SRC)
	gcovr --branches -e $(SRC)
	rm *.gcda *.gcno