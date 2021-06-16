#!/usr/bin/env python
# coding: utf-8

import subprocess
import os
import ujson as json
from time import time, sleep
import random
import matplotlib.pyplot as plt
# import matplotlib.pyplot as figure
import shlex
import sys
import random

def get_cpu_util(pid):
    process = subprocess.Popen(shlex.split('ps -p '+ str(pid) + ' -o %cpu'), encoding='utf-8', stdout=subprocess.PIPE)
    output = process.stdout.readlines()
    return float(output[1]);

def get_mem_util(pid):
    process = subprocess.Popen(shlex.split('ps -p ' + str(pid) + ' -o %mem'), encoding='utf-8', stdout=subprocess.PIPE)
    output = process.stdout.readlines()
    return float(output[1])

    
def run_command(command, period):
    args = shlex.split(command)
    start = time()
    process = subprocess.Popen(shlex.split(command), encoding='utf-8', stdout=subprocess.PIPE) 
    cnt = 0
    cpu_util = 0
    mem_util = 0
    pid = process.pid
    while(True):
        if process.poll() is None:
            end = time()
            if (end - start) > period:
                cnt += 1
                cpu_util += get_cpu_util(pid)
                cpu_util = cpu_util / cnt
                mem_util += get_mem_util(pid)
                mem_util = mem_util / cnt
                print('kill process after', str(end-start))
                process.kill()
                return cpu_util, mem_util
            else:
                temp = get_cpu_util(pid)
                cpu_util += temp
                temp = get_mem_util(pid)
                mem_util += temp
                cnt += 1
        else:
            print('Error caused by signal ' + str(-process.poll()))
            break
    # error 
    process.kill()
    return -1, -1;

def drawGraph(times, col, f_name, avg_cpu=None, avg_mem=None, textstr=None):
    assert(sorted(times) == times)

    if avg_cpu != None:
        mini = min(avg_cpu)
        maxi = max(avg_cpu)
        plt.figure(figsize=(15, 6), dpi=80)
        plt.plot((times), avg_cpu, color=col)
        plt.title(f_name)
        plt.ylim([mini-0.0005, maxi+0.0005])
        if textstr is not None:
            plt.text(0.02, 0.5, textstr, fontsize=14, transform=plt.gcf().transFigure)
        plt.savefig(OUTPUT_DIR + f_name)
        return
    if avg_mem != None:
        mini = min(avg_mem)
        maxi = max(avg_mem)
        plt.figure(figsize=(15, 6), dpi=80)
        plt.plot((times), avg_mem, color=col)
        plt.title(f_name)
        plt.ylim([mini-0.0005, maxi+0.0005])
        plt.savefig(OUTPUT_DIR + f_name)
        return
    
    
def simulate_trace(trace_file, accelerated, core):
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
    print(end)

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
        command = './fake-workload/build/workload/workload --buckets 1' 
        command = command + '--thread ' + str(core)
        if target_cpu - ALPHA > ALPHA:
            command = command + ' --cpu-util ' + str(target_cpu-ALPHA) 
       
#        command = command + ' --memory-size ' + str(int(target_mem*MACHINE_SIZE/1000000))
        print(command)
        
        # Run the resource-usage-generator
        cpu, mem = run_command(command, PERIOD)
        if(cpu < 0 or mem < 0):
            print('Error...')
            break;
        print(100*target_cpu, cpu, target_mem, mem)
        
        # Actual utilization returned and recorded
        mock_avg_cpu.append(cpu)
        mock_avg_mem.append(mem)
        # Calculate error
        cpu_err_margin += abs(cpu-target_cpu*100)
        mem_err_margin += abs(mem-target_mem)
        
        
  
    print('mock: ', len(mock_avg_cpu), len(mock_avg_mem))
    print('Error margin (CPU): ', cpu_err_margin, len(mock_avg_cpu), cpu_err_margin/len(mock_avg_cpu))
    print('Error margin: (MEM)', mem_err_margin, len(mock_avg_mem), mem_err_margin/len(mock_avg_mem))
    return times, avg_cpu, mock_avg_cpu, avg_mem, mock_avg_mem, cpu_err_margin/len(mock_avg_cpu), mem_err_margin/len(mock_avg_mem);


def drawCPU(true_cpu, mock_cpu, times, start, end, file_name, txt=None):
    drawGraph(times[start:end], avg_cpu=true_cpu[start:end], col=COLOR[TYPE], f_name='{}_cpu_true'.format(file_name), textstr=txt);
    drawGraph(times[start:end], avg_cpu=mock_cpu, col=COLOR[TYPE], f_name='{}_cpu_mock'.format(file_name), textstr=txt);

    
def drawMEM(true_mem, mock_mem, times, start, end, TRACE_FILE):
    drawGraph(times[start:end], avg_mem=true_mem[start:end], col=COLOR[TYPE], f_name='{}_mem_true'.format(file_name));
    drawGraph(times[start:end], avg_mem=mock_mem, col=COLOR[TYPE], f_name='{}_mem_mock'.format(file_name));

    
    
def find_file_to_simulate(seed):
    '''
    FIND A TASK FROM 4_PROD WITH MORE THAN 30 ENTRIES
    '''
    random.seed(seed)
    dir_list = os.listdir(INPUT_DIR)
    length = len(dir_list)
    n = random.randint(0, length-1)
    return dir_list[n]
    

        
def main(seed, core):
    # SET PARAMETERS
    global PERIOD
    global ALPHA
    PERIOD = 5     # seconds
    ALPHA = 1/100   # percentage of cpu the ./build process takes at mininum / 100 (to make it between [0, 1]

    global COLOR
    COLOR = {
        '1_free': 'green',
        '4_prod': 'red',
        '2_beb': 'blue',
        '3_mid': 'yellow',
        'default': 'black'
    }
    global TYPE
    TYPE = '1_free'

    global MACHINE_SIZE
    global INPUT_DIR
    global OUTPUT_DIR
    global TRACE_FILE_NAME

    MACHINE_SIZE = 32082000000 # B
    INPUT_DIR = './WorkloadExamples/' # Must end with /
    OUTPUT_DIR = './WorkloadResultExamples/' 

    # FIND FILE TO SIMULATE (OPTIONAL)
    #TRACE_FILE_NAME = find_file_to_simulate(seed)
    TRACE_FILE_NAME = 'file_169637062408_212549600003_3324'
    print(TRACE_FILE_NAME)

    # SIMULATE TRACE
    times, avg_cpu, mock_avg_cpu, avg_mem, mock_avg_mem, cpu_err, mem_err = simulate_trace(INPUT_DIR + TRACE_FILE_NAME, True, core)

    # DRAW GRAPH (OPTIONAL)
    text = "Period {}, cpu err {}, mem_err {}".format(PERIOD, cpu_err, mem_err) 
    drawCPU(avg_cpu, mock_avg_cpu, times, 0, len(times), TRACE_FILE_NAME+"__thrid", text);
#    drawMEM(avg_mem, mock_avg_mem, times, 0, len(times), TRACE_FILE_NAME);
    
if __name__ == '__main__':
    if len(sys.argv) == 1:
        print('Enter seed number and number of cores.')
    elif len(sys.argv) == 2:
        print('Enter number of cores.')
    else:
        main(sys.argv[1], sys.argv[2]);
    

