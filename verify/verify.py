import sys

args = sys.argv[1:]
infilename = args[0]
outfilename = args[1]

infile = open(infilename, 'r')
outfile = open(outfilename, 'r')

max_area_per_group = int(infile.readline())
infile.readline() # ".cell"

cell_area = {}

ncells = int(infile.readline())
for _ in range(0, ncells):
    cell_id, area = [int(s) for s in infile.readline().strip().split(' ')]
    cell_area[cell_id] = area

infile.readline() # ".net"

nnets = int(infile.readline())
nets = {}
for net_id in range(0, nnets):
    net_size = int(infile.readline())
    nets[net_id] = {int(s) for s in infile.readline().strip().split(' ')}

print('max group area = {}'.format(max_area_per_group))
print('num of cells = {}'.format(ncells))
print('num of nets = {}'.format(nnets))

print('CELL AREAS:')
print(cell_area)

print('NETS:')
print(nets)

###############

expected_cost = int(outfile.readline())
ngroups = int(outfile.readline())

group_of_cell = {}
for cell_id in range(0, ncells):
    group_of_cell[cell_id] = int(outfile.readline())


cost = 0
for net_id in range(0, nnets):
    groups_of_this_net = set()
    for cell_id in nets[net_id]:
        groups_of_this_net.add(group_of_cell[cell_id])
    span = len(groups_of_this_net)
    print('Net {} spans {} groups'.format(net_id, span))
    cost += (span - 1) * (span - 1)

print('expected cost = {}'.format(expected_cost))
print('cost = {}'.format(cost))

if cost == expected_cost:
    print('cost matches')
else:
    print('COST MISMATCH')
