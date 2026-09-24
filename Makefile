CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -I.
LIBS = -lwininet -lshlwapi

SRCS = src/main.cpp \
       src/ui.cpp \
       src/config.cpp \
       src/ignore_parser.cpp \
       src/git_engine.cpp \
       src/ftp_client.cpp \
       src/sync_engine.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = gftp.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del /f /q src\*.o $(TARGET)

.PHONY: all clean
