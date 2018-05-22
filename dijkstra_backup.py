#!/usr/bin/python
import sys
import subprocess

debug = False

def run_dijkstra(source_id, dest_id, msg):
    prev = []
    dist = []
    minpq = []
    for i in range(256):
        prev.append(-1)
        dist.append(INT_MAX)
        if i == source_id:
            dist[i] = 0
        t = (i, dist[i])
        minpq.append(t)

    while len(minpq) != 0:
        minpq.sort(key = lambda tup:tup[1], reverse = True)
        u = minpq.pop()
        #print "pop u: ", u
        for i in range(256):
            if adj_matrix[u[0]][i] == INT_MAX:
                #print"adj max", "i = ", i
                continue
            if dist[u[0]] == INT_MAX:
                #print"dist max", "i = ", i
                continue
            if u[0] == i:
                #print"myself", "i = ", i
                continue
            alt = dist[u[0]] + adj_matrix[u[0]][i]
            old_dist = dist[i]

            # Tie breakin
            has_better_path = alt < dist[i]
            has_better_path = has_better_path or ((alt == dist[i]) and (prev[i] == 1 or u[0] < prev[i]))
            if has_better_path:
                dist[i] = alt
                prev[i] = u[0]
                search = (i, old_dist)
                #print"minpq is ", minpq
                idx = minpq.index(search)
                update = (i, alt)
                minpq[idx] = update
                #print "dist[", i,"] ", " is updated to be ", dist[i]
            else:
                pass
                #print "No better path", "i = ", i

    path = []
    curr = dest_id
    while 1:
        path.insert(0, curr)
        curr = prev[curr]
        if prev[curr] == -1:
            path.insert(0, curr)
            break

    print "Generated path: ", path
    if len(path) == 0 or path[0] != source_id or path[len(path) - 1] != dest_id:
        print("Dijkstra failed to find shortest path")

        source_log_filename = testdir + "/correct_logs/log" + str(source_id) + ".txt"
        with open(source_log_filename, 'w+') as f:
            f.write("unreachable dest " + str(dest_id) + "\n")
        f.close()
        exit()

    # If path is found, generate logs here
    for i in range(len(path)):
        source_log_filename = testdir + "/correct_logs/log" + str(path[i]) + ".txt"
        if i == len(path) - 1:
            with open(source_log_filename, 'w+') as f:
                f.write("receive packet message " + msg + "\n")
            f.close()
        else:
            with open(source_log_filename, 'w+') as f:
                if i == 0:
                    f.write("sending packet dest " + str(dest_id) + " nexthop " + str(path[i+1]) + " message " + msg + "\n")
                else:
                    f.write("forward packet dest " + str(dest_id) + " nexthop " + str(path[i+1]) + " message " + msg + "\n")
            f.close()
    print("All logs written")
    print"dist is ", dist[dest_id]

example = '''
./dijkstra.py which_testdir command_file topology_file cost_file1 cost_file2 ....
For example:
./dijkstra.py lstest1 commands.txt topology.txt 1.txt 2.txt 8.txt '''

if len(sys.argv) < 5:
    print "Invalid arguments, try this: ", example
    exit()

# MAX COST in MP2
INT_MAX = 8388608

testdir = sys.argv[1]

# topology file
command_file = sys.argv[2]

# topology file
topo_file = sys.argv[3]

cost_files = []
for i in range(4, len(sys.argv)):
    cost_files.append(sys.argv[i])

for i in range(256):
    filename = testdir + '/correct_logs/log' + str(i) + '.txt'
    subprocess.Popen(['touch', filename])

################################################################################ Step1, make graph, read files and shit...
# Initialize all matrices
adj_matrix = []
# topology[i][j] == 1 means a connection between two nodes
topology = []
# cost[i][j] == 10 means link cost i->j is 10
cost_matrix = []
# First, build adj matrix with topology.txt and all initial costs
for i in range(256):
    adj_matrix.append([])
    topology.append([])
    cost_matrix.append([])
    for j in range(256):
        topology[i].append(0)
        if i == j:
            adj_matrix[i].append(0)
            cost_matrix[i].append(0)
        else:
            adj_matrix[i].append(INT_MAX)
            cost_matrix[i].append(1)

# Construct topology matrix
with open(topo_file) as f:
    topo_lines= f.readlines()
    f.close()
    for line in topo_lines:
        result = line.split()
        #print "Curr topo line: ", result
        if len(result) != 2:
            print "Parse failed"
            exit()
        topology[int(result[0])][int(result[1])] = 1
        topology[int(result[1])][int(result[0])] = 1

# Construct cost matrix

for i in range(len(cost_files)):
    with open(cost_files[i]) as f:
        cost_lines= f.readlines()
        f.close()
        slash_separated = cost_files[i].split('/')
        ret = slash_separated[len(slash_separated) - 1].split('.')
        source_id = int(ret[0])
        for line in cost_lines:
            result = line.split()
            if len(result) != 2:
                print "Parse failed"
                exit()
            cost_matrix[source_id][int(result[0])] = int(result[1])
# Now construct adj matrix
for i in range(256):
    for j in range(256):
        if topology[i][j] == 1:
            adj_matrix[i][j] = cost_matrix[i][j]

if 0:
    for i in range(256):
        for j in range(256):
            if topology[i][j] != 0:
                print "topo[" + str(i) + "]" + "[" +  str(j) + "] = " + str(topology[i][j])
if 0:
    for i in range(256):
        for j in range(256):
            if adj_matrix[i][j] != INT_MAX and adj_matrix[i][j] != 0:
                print "adj_matrix[" + str(i) + "]" + "[" +  str(j) + "] = " + str(adj_matrix[i][j])
################################################################################ Step2, make changes to graph, read commands...
'''
Command example
0 send 1 hello
0 cost 2 100
0 linkdown 2
0 linkup 8
1 sleep
'''
with open(command_file) as f:
    command_lines = f.readlines()
    print "command_lines is ", command_lines
    for command in command_lines:
        result = command.split()
        print("result command is ", result)
        if result[1] == "send":
            # Do dijkstra and write logs
            source_id = int(result[0])
            dest_id = int(result[2])
            msg = result[3]
            run_dijkstra(source_id, dest_id, msg)
        elif result[1] == "cost":
            source_id = int(result[0])
            dest_id = int(result[2])
            cost = int(result[3])
            cost_matrix[source_id][dest_id] = cost
        elif result[1] == "linkdown":
            source_id = int(result[0])
            dest_id = int(result[2])
            topology[source_id][dest_id] = 0
        elif result[1] == "linkup":
            source_id = int(result[0])
            dest_id = int(result[2])
            topology[source_id][dest_id] = 1
f.close()
exit()

