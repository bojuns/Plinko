from suffix_tree.tree import Tree
from suffix_tree.builder_factory import BUILDERS
import re

def maximal_repeats(tree, min_len=2, min_count=2):
    a = []
    for cv, path in sorted(tree.maximal_repeats()):
        if len(path) >= min_len:
            max_rep = str(path).replace(' ', '')
            if re.search('^[01]*1[01]*1[01]*$', max_rep): # all matches should be only 0s and 1s, and at least two 1s
                # strip leading and trailing 0s
                max_rep = re.sub(r'^0*(1[01]*1)0*$', r'\1', max_rep)
                count = len(tree.find_all(max_rep))
                if count >= min_count:
                    entry = (count, "%s" % max_rep)
                    if entry not in a and len(entry[1]) >= min_len:
                        a.append(entry)
    return a


def test_maximal_repeats(builder):
    # See [Gusfield1997]_ §7.12.1, 144ff.
    tree = Tree({"A": "1000101010101100101010101011101001011000100101011110000000000111001010001011010001011000111111001000010101010001001001000111010010110010$", 
                 'B': '10101010101001$'}, builder=builder)
    print(maximal_repeats(tree, 2))


def sort_by_first_elem(e):
    return e[0]


def sort_by_num_bits(e):
    return e[0] * (e[1].count('1') + e[1].count('0'))


def parse_pht_file(path):
    trace = open(path, 'r')
    lines = trace.readlines()
    entries = []

    for i in range(1, len(lines)):
        tok = re.split(" +", lines[i].strip())
        entries.append([int(tok[0]), tok[1]])

    entries.sort(key=sort_by_first_elem)
    base = entries[0][0]
    big_str = f"{entries[0][1]}"
    
    for i in range(2, len(lines)):
        diff = entries[i-1][0] - base
        base = entries[i-1][0]
        if diff > 1:
            big_str += "." * 64 # preserve 64B footprint alignment
        big_str += entries[i-1][1]

    return big_str, entries[0][0]


trace_str, base = parse_pht_file("/home/jkentw/Documents/CMU/2024spring/18742/project/src/results_4core/PHT_Snapshot_100000")

tree = Tree({"A": trace_str + "$"}, builder=BUILDERS[2])
reps = maximal_repeats(tree, 8, 4)
reps.sort(key=sort_by_first_elem, reverse=True)

out_file = open("/home/jkentw/Documents/CMU/2024spring/18742/project/src/results_4core/counts_100000.txt", "w")


for i in range(0, len(reps)):
    print(reps[i])
    out_file.write(str(reps[i]) + "\n")

print(reps[50][1])
occ = tree.find_all(reps[50][1])

for i in range(0, len(occ)):
    print(len(trace_str) - len(occ[i][1]) + 2)
#print(str(tree.find_all(reps[0][1])[0][1]).replace(' ', ''))
