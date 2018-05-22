#!/usr/bin/python
import subprocess
from time import sleep
import os
import random
import sys

def is_valid_id(node_id, dimension):
    if node_id < 10:
        return node_id <= dimension - 1
    left_digit = node_id / 10
    right_digit = node_id % 10
    return left_digit <= dimension - 1 and right_digit <= dimension - 1
# Test maker

test_name = sys.argv[1]
dimension = int(sys.argv[2])

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
node_list = []
for i in range(dimension):
    for j in range(dimension):
        node_id_str = str(i) + str(j)
        node_id = int(node_id_str)
        node_list.append(node_id)
        right = node_id + 1
        bottom = node_id + 10
        if is_valid_id(right, dimension):
            topo_list.append((node_id, right))
        if is_valid_id(bottom, dimension):
            topo_list.append((node_id, bottom))
print topo_list

with open(test_name + "/topology.txt", 'w+') as f:
    for i, topo_tuple in enumerate(topo_list):
        line = str(topo_tuple[0]) + " " + str(topo_tuple[1]) + '\n'
        f.write(line)
f.close()

# Cost is all 1's
for node in node_list:
    with open(test_name + "/cost/" + str(node) + ".txt", 'w+') as f:
        pass
    f.close()
