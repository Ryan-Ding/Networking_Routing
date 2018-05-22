#!/usr/bin/python
import subprocess
from time import sleep
import os
import random
import sys

# Test maker

num_nodes = int(sys.argv[1])
test_name = sys.argv[2]

# Create a directory for this test
os.system("rm -rf " + test_name)
os.system("mkdir " + test_name)
os.system("mkdir " + test_name + "/output_logs")
os.system("mkdir " + test_name + "/correct_logs")
os.system("mkdir " + test_name + "/cost")
os.system("touch " + test_name + "/commands.txt")


random.seed()

# Generate a random topology file, nodes between 0 to num_nodes
topo_list = []
num_edges = min(num_nodes * max(1,(num_nodes - 6)) / 3, 100)
for i in range(num_edges):
    n1 = random.randint(0, num_nodes - 1)
    n2 = random.randint(0, num_nodes - 1)
    if n1 == n2:
        continue
    topo_list.append((n1, n2))

# Remove duplicates
topo_list = list(set(topo_list))
#print "topo list"
#print topo_list

with open(test_name + "/topology.txt", 'w+') as f:
    for i, topo_tuple in enumerate(topo_list):
        line = str(topo_tuple[0]) + " " + str(topo_tuple[1]) + '\n'
        f.write(line)
f.close()

# Generate a random cost file for each of the nodes

for i in range(num_nodes):

    cost_list = []
    num_edges = num_nodes
    for j in range(num_edges):
        n1 = random.randint(0, num_nodes - 1)
        if n1 == i:
            continue
        n2 = random.randint(0, 50)
        cost_list.append((n1, n2))

    # Remove duplicates
    cost_list = list(set(cost_list))
    #print "cost list"
    #print cost_list

    with open(test_name + "/cost/" + str(i) + ".txt", 'w+') as f:
        for k, cost_tuple in enumerate(cost_list):
            line = str(cost_tuple[0]) + " " + str(cost_tuple[1]) + '\n'
            f.write(line)
    f.close()






