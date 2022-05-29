import yaml
import json
import os
import os.path
from functools import lru_cache
import csv

def readtxt(name):
    if os.path.exists(name):
        with open(name, 'r') as file:
            return [e.replace("\n", "") for e in file.readlines()]
    else:
        return []

def is_rw():
    return int(readtxt("../ips/is_rw")[0])

# the library for the data-processing
BASE="../"
BASE_REF_CO="/home/xuhao/copilot/"  # for ref.copilot
if is_rw(): # for rw
    SK="WRITE"
else:
    SK="PAYMENT"

# trials, by default: 1
FIGURE5a_TARIALS=3
FIGURE5b_TARIALS=3
FIGURE6a_TARIALS=3
FIGURE6b_TARIALS=3

def median(a):
    m=max(a)
    if m==0: return 0,0
    copy=[e for e in a] 
    for i in range(len(a)):
        if a[i]==0:
            a[i]=m
    v_i=0
    v=sorted(a)[len(a)//2]
    for i in range(len(copy)):
        if copy[i]==v:
            v_i=i
    return v,v_i

def convert_yml_json(name):
  with open(name, 'r') as file:
      configuration = yaml.safe_load(file)
  
  with open(name+".json", 'w+') as json_file:
      json.dump(configuration, json_file)

  return json.load(open(name+".json"))

def find_the_yml(folder_name):
    if not os.path.isdir(folder_name):
        return ""

    for filename in os.listdir(folder_name):
        f = os.path.join(folder_name, filename)
        if os.path.isfile(f):
            if ".yml" in filename and ".json" not in filename:
                return folder_name + "/" + filename
    return ""

@lru_cache(maxsize=None)
def figure5a():
    data_3_all, data_5_all=[],[]
    for i in range(FIGURE5a_TARIALS):
        data_3_t, data_5_t = figure5aTrail(i+1)
        data_3_all.append(data_3_t)
        data_5_all.append(data_5_t)

    data_3=data_3_all[0]
    data_5=data_5_all[0]
    H=len(data_3)

    for i in range(H):
        tmp_50th=[]
        tmp_tps=[]
        tmp_99th=[]
        tmp_all=[]
        for j in range(FIGURE5a_TARIALS):
            tmp_50th.append(float(data_3_all[j][i][1]))
            tmp_tps.append(float(data_3_all[j][i][2]))
            tmp_99th.append(float(data_3_all[j][i][3]))
            tmp_all.append(data_3_all[j][i][4])
        data_3[i][2],v_i = median(tmp_tps)
        data_3[i][1] = tmp_50th[v_i]
        data_3[i][3] = tmp_99th[v_i]
        data_3[i][4] = tmp_all[v_i]

    H=len(data_5)
    for i in range(H):
        tmp_50th=[]
        tmp_tps=[]
        tmp_99th=[]
        tmp_all=[]
        for j in range(FIGURE5a_TARIALS):
            tmp_50th.append(float(data_5_all[j][i][1]))
            tmp_tps.append(float(data_5_all[j][i][2]))
            tmp_99th.append(float(data_5_all[j][i][3]))
            tmp_all.append(data_5_all[j][i][4])
        data_5[i][2],v_i = median(tmp_tps)
        data_5[i][1] = tmp_50th[v_i]
        data_5[i][3] = tmp_99th[v_i]
        data_5[i][4] = tmp_all[v_i]
    return data_3, data_5

@lru_cache(maxsize=None)
def figure5aTrail(t):
    # 3 replicas
    if is_rw(): # rw
        conc=[20, 40, 60, 80, 100, 130, 160, 190, 200, 220, 260, 300, 340, 380, 420, 460, 500, 540, 580]
    else:  # tpca
        conc=[20, 40, 60, 80, 100, 130, 160, 190, 200, 220, 260, 300, 340, 380, 420]
    data_3=[] # (currency, 50th, tps, 99th, all_latency)
    for i in conc:
        folder=BASE+"figure5a_"+str(t)+"/results_3_"+str(i)
        if find_the_yml(folder):
            data=convert_yml_json(find_the_yml(folder))
            data_3.append([i, data[SK]["all_latency"]["50"], data[SK]["tps"], data[SK]["all_latency"]["99"], data[SK]["all_latency"]])
        else:
            data_3.append([i, 0, 0, 0, {str(i):0 for i in range(100)}])
            print("No yml found in " + folder)
    
    # 5 replicas
    data_5=[]
    for i in conc:
        folder=BASE+"figure5a_"+str(t)+"/results_5_"+str(i)
        if find_the_yml(folder):
            data=convert_yml_json(find_the_yml(folder))
            data_5.append([i, data[SK]["all_latency"]["50"], data[SK]["tps"], data[SK]["all_latency"]["99"], data[SK]["all_latency"]])
        else:
            data_5.append([i, 0, 0, 0, {str(i):0 for i in range(100)}])
            print("No yml found in " + folder)
    return data_3, data_5

@lru_cache(maxsize=None)
def figure5b():
    data_3_all, data_5_all=[],[]
    for i in range(FIGURE5b_TARIALS):
        data_3_t, data_5_t = figure5bTrail(i+1)
        data_3_all.append(data_3_t)
        data_5_all.append(data_5_t)

    data_3=data_3_all[0]
    data_5=data_5_all[0]
    H=len(data_3)

    for i in range(H):
        tmp_50th=[]
        tmp_tps=[]
        tmp_all=[]
        for j in range(FIGURE5b_TARIALS):
            tmp_50th.append(float(data_3_all[j][i][1]))
            tmp_tps.append(float(data_3_all[j][i][2]))
            tmp_all.append(data_3_all[j][i][3])
        data_3[i][2],v_i = median(tmp_tps)
        data_3[i][1] = tmp_50th[v_i]
        data_3[i][3] = tmp_all[v_i] 

    H=len(data_5)
    for i in range(H):
        tmp_50th=[]
        tmp_tps=[]
        tmp_all=[]
        for j in range(FIGURE5b_TARIALS):
            tmp_50th.append(float(data_5_all[j][i][1]))
            tmp_tps.append(float(data_5_all[j][i][2]))
            tmp_all.append(data_5_all[j][i][3])
        data_5[i][2],v_i = median(tmp_tps)
        data_5[i][1] = tmp_50th[v_i]
        data_5[i][3] = tmp_all[v_i]
    return data_3, data_5

@lru_cache(maxsize=None)
def figure5bTrail(t):
    # 3 replicas
    exp=[1, 2, 3, 4, 5, 6]
    data_3=[] # (exp, 50th, tps, all_latency)
    for i in exp:
        folder=BASE+"/figure5b_"+str(t)+"/results_3_"+str(i)
        if find_the_yml(folder):
            data=convert_yml_json(find_the_yml(folder))
            data_3.append([i, data[SK]["all_latency"]["50"], data[SK]["tps"], data[SK]["all_latency"]])
        else:
            data_3.append([i, 0, 0, {str(i):0 for i in range(100)}])
            print("No yml found in " + folder)
    
    # 5 replicas
    data_5=[]
    for i in exp:
        folder=BASE+"/figure5b_"+str(t)+"/results_5_"+str(i)
        if find_the_yml(folder):
            data=convert_yml_json(find_the_yml(folder))
            data_5.append([i, data[SK]["all_latency"]["50"], data[SK]["tps"], data[SK]["all_latency"]])
        else:
            data_5.append([i, 0, 0, {str(i):0 for i in range(100)}])
            print("No yml found in " + folder)

    return data_3, data_5

@lru_cache(maxsize=None)
def figure6a_copilot():  # 1 trail is enough
    clients=[int(e) for e in "4 8 12 16 20 24 28 32 36 40 44 48 52 56 60 64".split(" ")]
    data=[] # (# of clients, tps, all_latency)
    for c in clients:
        # # of clients
        items = [c]
        
        # tps
        filename=BASE_REF_CO+"figure6a_copilot_1/experiments-"+str(c)+"/latest/tput.txt"
        if os.path.exists(filename):
            items.append(float(readtxt(filename)[0]))
        else:
            items.append(0)

        # all_latency
        filename=BASE_REF_CO+"figure6a_copilot_1/experiments-"+str(c)+"/latest/tputlat.txt"
        all_latency = []
        if os.path.exists(filename):
            values = readtxt(filename)[0].split("\t")
            items.append([float(values[i+1]) for i in range(99)])
        else:
            items.append([0 for i in range(99)])

        data.append(items)    
    return data

@lru_cache(maxsize=None)
def figure6a():
    data_r_all = []
    for i in range(FIGURE6a_TARIALS):
        data_r_t = figure6aTrail(i+1)
        data_r_all.append(data_r_t)

    data_r=data_r_all[0]
    H=len(data_r)

    for i in range(H):
        tmp_50th=[]
        tmp_tps=[]
        tmp_99th=[]
        tmp_all=[]
        for j in range(FIGURE6a_TARIALS):
            tmp_50th.append(float(data_r_all[j][i][1]))
            tmp_tps.append(float(data_r_all[j][i][2]))
            tmp_99th.append(float(data_r_all[j][i][3]))
            tmp_all.append(data_r_all[j][i][4])
        data_r[i][2],v_i = median(tmp_tps)
        data_r[i][1] = tmp_50th[v_i]
        data_r[i][3] = tmp_99th[v_i]
        data_r[i][4] = tmp_all[v_i]

    return data_r

@lru_cache(maxsize=None)
def figure6aTrail(t):
    if is_rw():
        conc=[5, 10, 15, 20, 30, 40, 50, 60, 80, 100]
    else:
        conc=[1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20]
    data_r=[] # (currency, 50th, tps, 99th, all_latency)
    for i in conc:
        folder=BASE+"/figure6a_"+str(t)+"/results_"+str(i)
        if find_the_yml(folder):
            data=convert_yml_json(find_the_yml(folder))
            data_r.append([i, data[SK]["all_latency"]["50"], data[SK]["tps"], data[SK]["all_latency"]["99"], data[SK]["all_latency"]])
        else:
            data_r.append([i, 0, 0, 0, {str(i):0 for i in range(100)}])
            print("No yml found in " + folder)
    
    return data_r

@lru_cache(maxsize=None)
def figure6b():
    data_l_all, data_f_all = [],[]
    for i in range(FIGURE6b_TARIALS):
        data_l_t, data_f_t = figure6bTrail(i+1)
        data_l_all.append(data_l_t)
        data_f_all.append(data_f_t)

    data_l=data_l_all[0]
    data_f=data_f_all[0]
    H=len(data_l)

    for i in range(H):
        tmp_50th=[]
        tmp_tps=[]
        tmp_all=[]
        for j in range(FIGURE6b_TARIALS):
            tmp_50th.append(float(data_l_all[j][i][1]))
            tmp_tps.append(float(data_l_all[j][i][2]))
            tmp_all.append(data_l_all[j][i][3])
        data_l[i][2],v_i = median(tmp_tps)
        data_l[i][1] = tmp_50th[v_i]
        data_l[i][3] = tmp_all[v_i]

    H=len(data_f)
    for i in range(H):
        tmp_50th=[]
        tmp_tps=[]
        tmp_all=[]
        for j in range(FIGURE6b_TARIALS):
            tmp_50th.append(float(data_f_all[j][i][1]))
            tmp_tps.append(float(data_f_all[j][i][2]))
            tmp_all.append(data_f_all[j][i][3])
        data_f[i][2],v_i = median(tmp_tps)
        data_f[i][1] = tmp_50th[v_i]
        data_f[i][3] = tmp_all[v_i]

    return data_l, data_f

@lru_cache(maxsize=None)
def figure6bTrail(t):
    # leader
    exp=[1, 2, 5, 6]
    data_l=[] # (exp, 50th, tps, all_latency)
    for i in exp:
        folder=BASE+"/figure6b_"+str(t)+"/results_leader_"+str(i)
        if find_the_yml(folder):
            data=convert_yml_json(find_the_yml(folder))
            data_l.append([i, data[SK]["all_latency"]["50"], data[SK]["tps"], data[SK]["all_latency"]])
        else:
            data_l.append([i, 0, 0, {str(i):0 for i in range(100)}])
            print("No yml found in " + folder)
    
    # follower
    data_f=[]
    for i in exp:
        folder=BASE+"/figure6b_"+str(t)+"/results_follower_"+str(i)
        if find_the_yml(folder):
            data=convert_yml_json(find_the_yml(folder))
            data_f.append([i, data[SK]["all_latency"]["50"], data[SK]["tps"], data[SK]["all_latency"]])
        else:
            data_f.append([i, 0, 0, {str(i):0 for i in range(100)}])
            print("No yml found in " + folder)

    return data_l, data_f


if __name__ == "__main__":
    #print(figure5a())
    #print(figure5b())
    #print(figure6a())
    #print(figure6b())
    #print(figure6a_copilot())
    pass
