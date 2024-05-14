import sys
import getopt
import subprocess
import os
import pandas as pd
import matplotlib.pyplot as plt

def calculate_latency_metrics(logfile, concurrent, req):

    filepath = f'./plots/quepaxa/{logfile}_{concurrent}_{req}.csv'
    data = pd.read_csv(filepath, header=None)
    latencies = data.stack().astype(float)
    median_latency = latencies.median() * 1000
    
    return median_latency

def main(argv):
    inputfile = ''
    outputfile = ''
    opts, args = getopt.getopt(argv,'n:T:')
    
    throughputs = [100000, 200000, 300000, 400000, 500000, 600000]
    n_concurrent = 10000
    latency_results = []

    for throughput in throughputs:
        cmd = ['build/deptran_server', '-f', 'config/quepaxa_test.yml', '-n', str(n_concurrent), '-T', str(throughput)]  # `-T` requires just the number
        print('Executing command: ' + ' '.join(cmd))
        with open('out.log', 'w') as outfile:
            subprocess.run(cmd, stdout=outfile, check=True)
        
        median_latency = calculate_latency_metrics("latencies", n_concurrent, throughput)
        latency_results.append(median_latency)
        print(f'Median Latency for {throughput}: {median_latency} ms')

    plt.figure(figsize=(10, 6))

    throughput_labels = [f"{int(t / 1000)}K" for t in throughputs]

    plt.plot(throughput_labels, latency_results, marker='o', label='QuePaxa')

    plt.xlabel('Throughput (x 1k requests)')
    plt.ylabel('Median Latency (ms)')
    plt.title('Median Latency vs. Throughput')

    plt.legend()
    plt.grid(True)

    plt.savefig('./plots/median_latency_vs_throughput.png')

    plt.show()

if __name__ == '__main__':
    main(sys.argv[1:])
