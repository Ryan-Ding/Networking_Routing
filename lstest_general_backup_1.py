#!/usr/bin/python
import subprocess
from time import sleep
import os
import sys
import filecmp


test_name = sys.argv[1]



# Setup environment
print "Setting up environment.."
setup = subprocess.Popen(['perl', 'make_topology.pl', test_name + '/topology.txt'])
setup.wait()

os.system("rm -rf " + test_name + '/output_logs/*')
os.system("rm -rf " + test_name + '/correct_logs/*')


# Start three routers
print "#################################################### Now run ls_router ################################################"
cost_files = os.listdir(test_name + "/cost")

nodes_list = []
with open(test_name + '/topology.txt') as f:
    topo_lines = f.readlines()
    for line in topo_lines:
        ret = line.split()
        nodes_list.append(ret[0])
        nodes_list.append(ret[1])
f.close()
nodes_list = list(set(nodes_list))

for node in nodes_list:
    source_id = str(node)
    cost_filename = test_name + "/cost/" + source_id + ".txt"
    argument_list = ['./ls_router', source_id, cost_filename, test_name + '/output_logs/log' + source_id + ".txt"]
    subprocess.Popen(argument_list)

cost_files_copy = []
for i in range(len(cost_files)):
    cost_files_copy.append(test_name + "/cost/" + cost_files[i])

print "Wait 5 seconds for everything to converge.."
sleep(5)

# Read command.txt line by line
with open(test_name + "/commands.txt") as f:
    command_lines = f.readlines()
    for command in command_lines:
        result = command.split()
        print("result command is ", result)
        if result[1] == "send":
            # Do dijkstra and write logs
            source_id = result[0]
            dest_id = result[2]
            msg = result[3]
            manager = subprocess.Popen(['./manager_send', source_id, 'send', dest_id, msg])
            manager.wait()
        elif result[1] == "cost":
            source_id = result[0]
            dest_id = result[2]
            cost = result[3]
            manager = subprocess.Popen(['./manager_send', source_id, 'cost', dest_id, cost])
        elif result[1] == "linkdown":
            source_id = result[0]
            dest_id = result[2]
            source_ip = "10.1.1." + source_id
            dest_ip = "10.1.1." + dest_id
            p = subprocess.Popen(['iptables', '-D', 'OUTPUT', '-s', source_ip, '-d', dest_ip, '-j', 'ACCEPT'])
            p.wait()
            p = subprocess.Popen(['iptables', '-D', 'OUTPUT', '-s', dest_ip, '-d', source_ip, '-j', 'ACCEPT'])
            p.wait()
        elif result[1] == "linkup":
            source_id = result[0]
            dest_id = result[2]
            source_ip = "10.1.1." + source_id
            dest_ip = "10.1.1." + dest_id
            p = subprocess.Popen(['iptables', '-I', 'OUTPUT', '-s', source_ip, '-d', dest_ip, '-j', 'ACCEPT'])
            p.wait()
            p = subprocess.Popen(['iptables', '-I', 'OUTPUT', '-s', dest_ip, '-d', source_ip, '-j', 'ACCEPT'])
            p.wait()
        elif result[1] == "sleep":
            duration = float(result[0])
            sleep(duration)
f.close()

sleep(5)

print "#################################################### Now run solution ################################################"
# Give all the topology files and cost files to dijkstra.py, it should produce the correct logs

killall = subprocess.Popen(['killall', 'ls_router'])
killall.wait()

dijkstra = subprocess.Popen(['./dijkstra.py', test_name, test_name + '/commands.txt', test_name + '/topology.txt'] + cost_files_copy)
dijkstra.wait()
print "Now diff dijkstra result against ls_router's.."


for node in nodes_list:
    source_id = str(node)
    file1 = test_name + '/output_logs/log' + source_id +  '.txt'
    file2 = test_name + '/correct_logs/log' + source_id + '.txt'
    compare = filecmp.cmp(file1, file2)
    if compare:
        with open(file1) as f:
            ret = f.read()
            if ret == "":
                # print "(empty)"
                # print "log" + source_id + ".txt matched (empty)"
                pass
            else:
                print "log" + source_id + ".txt matched (not empty)"
                print
                # print ret
    else:
        print "log" + source_id + ".txt differed"
        print "correct:"
        with open(file2) as f:
            ret = f.read()
            if ret == "":
                print "(empty)"
            else:
                print ret
        f.close()
        print "your output:"
        with open(file1) as f:
            ret = f.read()
            if ret == "":
                print "(empty)"
            else:
                print ret
        print

        f.close()
