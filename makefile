concurrentSystemMonitor: stringUtils.o parseArguments.o parseCpuStats.o printSystem.o parseMemoryStats.o printUsers.o a3.o 
	gcc stringUtils.o parseArguments.o parseCpuStats.o printSystem.o parseMemoryStats.o printUsers.o a3.o -Wall -o concurrentSystemMonitor

%.o: %.c
	gcc -c -o $@ $< -Wall

.PHONY: clean

clean:
	rm -f stringUtils.o parseArguments.o parseCpuStats.o printSystem.o parseMemoryStats.o printUsers.o a3.o

.PHONY: cleandist

cleandist:
	rm -f stringUtils.o printTables.o processes.o readFileDescriptors.o readProcesses.o stringUtils.o main.o concurrentSystemMonitor

