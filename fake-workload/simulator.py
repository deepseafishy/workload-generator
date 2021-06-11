#!/usr/bin/env python
# coding: utf-8

# TODO in the future:
# make this interactive
# change poll to wait with a timeout
# read directly from /proc

import subprocess
import os
import ujson as json
from time import time, sleep
import random
import matplotlib.pyplot as plt
import matplotlib.pyplot as figure
import shlex

def get_cpu_util(pid):
    process = subprocess.Popen(shlex.split('ps -p '+ str(pid) + ' -o %cpu'), encoding='utf-8', stdout=subprocess.PIPE)
    output = process.stdout.readlines()
    return float(output[1]);

    
def run_command(command, period):
    args = shlex.split(command)
    start = time()
    process = subprocess.Popen(shlex.split(command), encoding='utf-8', stdout=subprocess.PIPE) 
    cnt = 0
    cpu_util = 0
    pid = process.pid
    print(pid)
    while(True):
        #print(process.stdout.readline())
        if process.poll() is None:
            end = time()
            if (end - start) > period:
                cpu_util += get_cpu_util(pid)
                cnt += 1
                cpu_util = cpu_util / cnt
                print('kill process after', str(end-start), 'seconds || ', cpu_util)
                process.kill()
                mem_util = 0
                return cpu_util, mem_util
            else:
                temp = get_cpu_util(pid)
                cpu_util += temp
                cnt += 1
        
    # error 
    return -1, -1;

def drawGraph(times, col, f_name, avg_cpu=None, avg_mem=None):
    assert(sorted(times) == times)

    if avg_cpu != None:
        mini = min(avg_cpu)
        maxi = max(avg_cpu)
        plt.figure(figsize=(15, 6), dpi=80)
        plt.plot((times), avg_cpu, color=col)
        plt.title(f_name + " avg_cpu")
#             plt.xlin([0, 8000])
        plt.ylim([mini-0.0005, maxi+0.0005])
#         plt.show()
        plt.savefig(f_name)
    if avg_mem != None:
        plt.figure(figsize=(15, 6), dpi=80)
        plt.plot((times), avg_mem, color=col)
        plt.title(f_name + " avg_mem")
#             plt.xlin([0, 8000])
        plt.ylim([mini-0.0005, maxi+0.0005])
#         plt.show()
        plt.savefig('plot-avg_mem.png')
    
    
def simulate_trace(trace_file, accelerated):
    '''
    SIMULATE WORKLOAD
    input:
        - trace_file (string): name of workload to be simulated
        - accelerated (boolean): specify whether simulation will start from absolute time 0 or time[0] specified in the workload
            Ex: The workload that we want to reproduce could start from time 100. This would mean that this workload didn't exist during the first 100 cycles. accelerated = False will simulate even this part. This would be useful if you would want to turn this generator on for multiple tasks that start at different time cycles, all at the creation of a VM. accelerated = True will skip this absence of activity and assume that the generator itself is turned on at the correct moment (times[i] = 100 means that right now 100th cycle is being executed).
     output: 
         Nothing
    '''
    # GET NECESSARY DATA (TIME, AVG_CPU, AVG_MEM) FROM FILE
    open_file = open(trace_file)
    file = json.load(open_file)
    times = file['time']
    avg_cpu = file['avg_cpu']
    avg_mem = file['avg_mem']
    mock_avg_cpu = []
    mock_avg_mem = []
    
    # False: assume generator is turned on at absolute time 0
    # True: assume generator is turned on at times[0]
    if accelerated is False:
        for i in range(0, times[0]+1):
            sleep(PERIOD)
    
    # Specify the portion [start, end) of the workload that would be simulated
    start = 0
    end = len(times)
    
    cpu_err_margin = 0
    mem_err_margin = 0

    # For each time step, simulate the CPU, MEM usage specified in workload
    for it, _t in enumerate(times):
        if it < start:
            continue;
        if it == end:
            break;
        
        # CPU, MEM usage that we want to simulate
        target_cpu = avg_cpu[it]
        target_mem = avg_mem[it]
        
        
        # If you don't want to simulate either of CPU or MEM, comment out appropriately:
        command = '../fake-generator/build/workload/workload --threads 2 --buckets 1' 
        command = command + '--cpu-util ' + str(target_cpu) 
        command = command + ' --memory ' + str(int(target_mem*100000))
        print(command)
        
        # Run the resource-usage-generator
        cpu, mem = run_command(command, PERIOD)
        # Actual utilization returned and recorded
        mock_avg_cpu.append(cpu)
        mock_avg_mem.append(mem)
        cpu_err_margin += abs(cpu-target_cpu*100)
        mem_err_margin += abs(mem-target_mem*100) # ??????????????????????///
        
        if(cpu < 0):
            print('Error...')
            break;
            
    print('mock: ', len(mock_avg_cpu), len(mock_avg_mem))
    print('Error margin: ', cpu_err_margin, len(mock_avg_cpu), cpu_err_margin/len(mock_avg_cpu))
    drawGraph(times[start:end], avg_cpu=avg_cpu[start:end], col='red', f_name='true_short');
    drawGraph(times[start:end], avg_cpu=mock_avg_cpu, col='red', f_name='mock_short');
    return;


def find_file_to_simulate():
    '''
    FIND A TASK FROM 4_PROD WITH MORE THAN 30 ENTRIES
    '''
    directory = '4_prod'
    i = 0
    for d in os.listdir('/mnt/temp/data/'+directory):
        open_file = open('/mnt/temp/data/'+directory+"/"+d)
        file = json.load(open_file)
        avg_cpu = file['avg_cpu']
        maxi = max(avg_cpu)
        mini = min(avg_cpu)
        if maxi-mini>0.1:
            f = '/mnt/temp/data/'+directory+"/"+d
            return f;


        
        
def main():
    # SET PARAMETERS
    global PERIOD
    PERIOD = 15 # seconds
    global color
    color = {
        '1_free': 'green',
        '4_prod': 'red',
        '2_beb': 'blue',
        '3_mid': 'yellow',
        'default': 'black'
    }
    TRACE_FILE = '' # CHANGE TO FILE NAME YOU WANT TO REPRODUCE
    
    # FIND FILE TO SIMULATE (OPTIONAL)
    trace_file = find_file_to_simulate()
    print(trace_file)
    
    # SIMULATE TRACE
    
    simulate_trace(trace_file, True)

    # DRAW GRAPH (OPTIONAL)
    
    
if __name__ == '__main__':
    main();
    

##########################################################################
