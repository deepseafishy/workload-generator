# Purpose
This is a workload generator that generates a workload based on input data. It can reproduce the CPU, memory utilization of a single task (or any number of tasks as long as the data is input in the correct format).
We are trying to make the CPU utilization of the machine running this generator similar to that specified in the input data.

# How to use
1. ``` python simulator.py ```

# Input data format
The workload data file should be a json file and must have the following 3 properties:
- time: array of floating point numbers representing the time 
- avg_cpu: array of floating point numbers representing the CPU utilization (in %) out of the machine's CPU capacity.
- avg_mem: array of floating point numbers representing the MEM utilization (in %) out of the machine's memory capacity.
Example:
```
{
  'time': [2562.0, 2563.0, 2564.0, 2565.0], 
  'avg_cpu': [0.00022983551025390625, 0.0002155303955078125, 0.0002193450927734375, 0.00021266937255859375], 
  'avg_mem': [0.000614166259765625, 0.0006284713745117188, 0.00061798095703125, 0.00061798095703125], 
  ...
}
```
# Possible configurations
- Change parameters:
  - The following global parameters can be changed in the ```main()``` function:
    - INPUT_DIR: Directory name of where the workload file is saved
    - TRACE_FILE_NAME: Name of workload file (searched randomly given seed)
    - OUTPUT_DIR: Directory name of where the graphs should be saved
    - PERIOD: time it takes in seconds to simulate one time step (for Google trace, it should be 5min since each time step represents 5min in real life. However, this is unrealistic for a simulator so we can change this to 10~15 seconds). A longer period is found to give better results.
    - COLOR: The color of the graph
    - MACHINE_SIZE: Size of the machine (in Bytes).

- Comment out some functions
  - Some functions such as drawMEM(), drawCPU(), find_file_to_simulate() can be commented out if they are not needed. However, if you disable find_file_to_simulate(), you MUST write the workload file's name in TRACE_FILE.
- accelerated
  - simulate_trace() function receives 'accelerated' as an input parameter. The value of this can be changed. Please refer to the comments in the code for more information.

# Dependencies
The following modules are used in the code:
- subprocess
- os
- ujson 
- time
- random
- matplotlib.pyplot
- shlex
