all: server

server: server.o driveSim.o LabCarClient.o GPIO.o
	g++ -w -o server server.o driveSim.o LabCarClient.o GPIO.o

server.o: server.cpp
	g++ -w -c server.cpp

driveSim.o: driveSim.cpp
	g++ -w -c driveSim.cpp

LabCarClient.o: LabCarClient.cpp
	g++ -w -c LabCarClient.cpp

GPIO.o: GPIO.cpp
	g++ -w -c GPIO.cpp

.PHONY: clean

clean:
	rm -rf *.o server
