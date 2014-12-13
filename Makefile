CXX=clang++
CC=clang
BUILDDIR=build
COMMONFLAGS=-Wall -Wextra -g -Iinclude/ `mysql_config --include`
CXXFLAGS=-std=c++11 $(COMMONFLAGS)
LINKFLAGS=-lpthread -lfcgi `mysql_config --libs`

all:
	mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o $(BUILDDIR)/main.o
	$(CXX) $(CXXFLAGS) -c src/misc.cpp -o $(BUILDDIR)/misc.o
	$(CXX) $(CXXFLAGS) -c src/Request.cpp -o $(BUILDDIR)/Request.o
	$(CXX) $(CXXFLAGS) -c src/ThreadEngine.cpp -o $(BUILDDIR)/ThreadEngine.o
	$(CXX) $(CXXFLAGS) $(BUILDDIR)/*.o -o $(BUILDDIR)/main $(LINKFLAGS)

clean:
	rm -rf $(BUILDDIR)
