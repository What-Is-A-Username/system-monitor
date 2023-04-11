# Linux System Monitoring Tool
 
This is a program written in C which summarizes the current state of the Linux system by presenting current system usage.

## Approach

The main process will launch three child processes, each reporting a different category of system information: CPU utilization, memory utilization, and connected users. These child processes will communicate back their findings to the parent process by using pipes.

## Installation

This tool only works for Linux machines. This installation assumes that you have already installed a GNU C++ compiler.

Compile the code:
```
g++ a1.c -o ./concurrentSystemMonitor
```

## Flags

### `--samples`

Determines the number of iterations that information is sampled for and printed by the tool. The delay between iterations is specified by the [`--tdelay` argument](#tdelay). **Default = 10**.

This value can be set either as a named command line argument (`--name=N`, where `N` is the new value) or as the first positional argument. In the event that two values are specified, the value stated last is taken. The value must be greater than zero or it will result in an error otherwise. 

Examples:
```
# Set samples to 8 using a named argument
./concurrentSystemMonitor --samples=8
# Set to 8 using a positional argument
./concurrentSystemMonitor 8
# Positional arguments can be combined with named arguments
./concurrentSystemMonitor 8 --graphics
./concurrentSystemMonitor --graphics 8

# If specified using two methods, the value appearing last is used. These commands all set samples to 5:
./concurrentSystemMonitor --samples=12 5
./concurrentSystemMonitor 11 --samples=5
```

### `--tdelay`

Specifies the number of seconds of delay between consecutive samples. **Default = 1**.

This value can be set using either as a named command line argument (`--tdelay=N`, where `N` is the new value) or as the second positional argument. The value be greater than zero and will result in an error otherwise.

Examples:
```
# Set time delay to 2 seconds using a named argument
./concurrentSystemMonitor --tdelay=2
# Set tdelay to 2 and samples to 1 using positional arguments
./concurrentSystemMonitor 1 2
# Positional arguments can be combined with named arguments
./concurrentSystemMonitor 1 2 --graphics
./concurrentSystemMonitor 1 --graphics 2
./concurrentSystemMonitor --graphics 1 2

# If specified using two methods, the value appearing last is used. These commands all set tdelay to 3:
./concurrentSystemMonitor --tdelay=2 1 3
./concurrentSystemMonitor 2 2 --samples=3
```

### `--system`

Indicate to only display the system usage information. If set then only display:
-   memory utilization
-   processor and core count
-   CPU utilization

In the event that both `--system` and `--user` are specified, both will be printed. **Default = false**.

For memory utilization a single line is printed for each sample, consisting of four numbers: (i) used physical memory, (ii) total physical memory, (iii) used virtual memory and (iv) total virtual memory.

Processor and core counts are based on data in the `/proc/cpuinfo` file. The number reported as `Number of processors` corresponds to the number of unique `physical id`s found in this file, while `Total number of cores` value is the sum of the sibling counts belonging to these unique ids. In this way, the total number of cores accounts for hyperthreading.

For CPU utilization, a single line is printed for each sample. The first value is CPU percentage utilization that has occurred since the previous sample. For the first sample, the "previous sample" data is data gathered [`tdelay`](#tdelay) seconds before. For all samples, the relative absolute change (`Relative Abs. Change`) is also calculated, corresponding to the difference from the current to the previous sample. The first sample's change is always zero.

The procedure to calculate these statistics is described under [Memory Utilization Calculations](#memory-utilization-calculations) and [CPU Utilization Calculations](#cpu-utilization-calculation) in this document.

### `--user`

Indicate to only display the user usage statistics. This shows the users currently connected and their active processes/sessions.

In the event that both `--system` and `--user` are specified, both will be printed. **Default = false**.

Information on current users and sessions is obtained from `getutent()` from [`getutent(3)`](https://man7.org/linux/man-pages/man3/getutent.3.html). Only sessions that are user processes are included. 

The output is presented as two columns. The first column indicates names of users (`utmp.ut_user`), and second column indicates the device name and remote login host name/address as reported by `utmp.ut_line` and `utmp.ut_host` respectively.

Example:
```
./concurrentSystemMonitor --user
```

### `--graphics`

If set, then graphic representations will be printed alongside memory and CPU utilization statistics. **Default = false**.

For each memory utilization sample, the change in used virtual memory of the current sample relative to the previous sample is represented as a bar beginning with `|`. An increase in memory utilization is indicated by a number of `#` characters proportional to the amount of increase and terminated by `*`. A decrease is indicated similarly, but with `:` and `@` characters instead. The value of the change in virtual memory is then printed. Another, final, number corresponds to the used virtual memory of the current sample.

For each CPU utilization sample, a graphical representation is added beginning with a `[` character. It is then followed by a number of `|` characters proportional to the current sample's CPU utilization. The final value printed is the current sample's CPU percentage utilization.

Example:
```
./concurrentSystemMonitor --graphics
```

### `--sequential`

If set, the output will be printed sequentially with no screen refresh functionality. **Default = false**

The output is made sequential by disabling the printing of ANSI escape codes and `ncurses.h` functionalities that reset the screen output and/or cursor.

This is helpful if the output is to be redirected elsewhere, such as to a file. While output can be redirected even without using `--sequential`, it will include redundant escape characters.

Example:
```
./concurrentSystemMonitor --sequential

# redirect to file
./concurrentSystemMonitor --sequential > out.txt
```

## Memory Utilization Calculations

This tool calculates memory utilization in the form of four values: Physical Memory Total, Physical Memory Used, Virtual Memory Total, Total Virtual Memory Used. The calculations depend upon the sysinfo data calculated by `sysinfo()` from [`sysinfo(2)`](https://man7.org/linux/man-pages/man2/sysinfo.2.html#DESCRIPTION).

**Physical Memory Total** corresponds to the amount of physical memory on the machine, as calculated by the `sysinfo.totalram` statistic.

**Physical Memory Used** is calculated by subtracting the available memory from the physical memory total described above. Available memory is the `sysinfo.freeram` statistic.

**Virtual Memory Total** corresponds to the amount of physical memory plus swap space on the machine. It is the sum of physical memory total (calculated above), and the total swap space size, which is `sysinfo.totalswap`.

**Virtual Memory Used** corresponds to the amount of virtual memory total that is currently being used. It is calculated by subtracting available swap space, which is `sysinfo.freeswap`.

## CPU Utilization Calculation

CPU utilization for each sample is calculated by taking two data points taken [`tdelay`](#tdelay) seconds apart from [`/proc/stat`](https://man7.org/linux/man-pages/man5/proc.5.html). Each data point consists of 10 values: `user`, `nice`, `system`, `idle`, `iowait`, `irq`, `softirq`, `steal`, `guest`, and `guest_nice` directly read from the first row of the file.

Suppose data point *t<sub>1</sub>* was taken tdelay seconds after data point *t<sub>0</sub>*. Let *user<sub>0</sub>* represent the `user` value for *t<sub>0</sub>*, *user<sub>1</sub>* represent the same for *t<sub>1</sub>*, and the other values be similarly defined for `nice`, `system`, etc. Then we can calculate total CPU time between the two data points (*total*) and the total idle time during the same period (*idle*) using the following formulas:

<p style="text-align: center;"><em>idle = idle<sub>1</sub> - idle<sub>0</sub> </em></p>

<p style="text-align: center;"><em>total = user<sub>1</sub> - user<sub>0</sub> + nice<sub>1</sub> - nice<sub>0</sub> + system<sub>1</sub> - system<sub>0</sub> + idle + iowait<sub>1</sub> - iowait<sub>0</sub> + irq<sub>1</sub> - irq<sub>0</sub> + softirq<sub>1</sub> - softirq<sub>0</sub> + steal<sub>1</sub> - steal<sub>0</sub></em></p>

Then percentage CPU utilization between the *t<sub>0</sub>* and *t<sub>1</sub>* can be calculated as:
<p style="text-align: center;"><em>CPU<sub>%</sub> = 1 - idle / total</em></p>

Note that these calculations still hold for the first CPU utilization sample recorded, since the first data point *t<sub>0</sub>* is taken immediately and the program has a delay of `tdelay` seconds before taking the second data point and outputting the first sample.