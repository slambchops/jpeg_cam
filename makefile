CC=g++
CFLAGS=-c -Wall
LDFLAGS=-ljpeg
SOURCES=main.cpp camera.cpp jpeg.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=jpeg_cam

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o $(EXECUTABLE)
