from suffix_tree.tree import Tree
from suffix_tree.builder_factory import BUILDERS
import re
import sys

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

# sorting "kernels"
def sort_by_first_elem(e):
    return e[0]

def sort_by_num_bits(e):
    return e[0] * (e[1].count('1') + e[1].count('0'))

# converts PHT trace into a single string for processing
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


def footprint_offset_stats(inFileName, outFileName, min_substring_len=8, min_occ=4, footprint_len=64):
    # parse PHT trace and get most frequent substrings
    trace_str, base = parse_pht_file(inFileName)
    tree = Tree({"A": trace_str}, builder=BUILDERS[2])
    reps = maximal_repeats(tree, min_substring_len, min_occ)
    reps.sort(key=sort_by_num_bits, reverse=True) # sort repeated substrings by total number of bits (freq*len)

    out_file = open(outFileName, "w")
    out_file.write("Frequency, Substring, Length, Zeroes, Ones") # write headers (CSV format)

    for i in range(0, footprint_len):
        out_file.write(", " + str(i)) # write offset headers (CSV format)
    out_file.write("\n")

    # for each substring...
    for i in range(0, len(reps)):
        occ = tree.find_all(reps[i][1]) # find all occurrences of substring
        offset_counts = [0] * footprint_len # initialize counter

        # find distribution of footprint offsets for this substring (iterate over all occurrences)
        for j in range(0, len(occ)):
            # index of occurrence is length of trace_string minus length of suffix 
            # (excluding internally-appended '$' character)
            str_offset = len(trace_str) - (len(occ[j][1]) - 1)

            # get offset relative to footprint
            footprint_offset = (str_offset + base) % footprint_len

            # update respective counter
            offset_counts[footprint_offset] += 1

        # write line in CSV format
        out_file.write(str(reps[i][0]) + ", \'" + str(reps[i][1]) + "\', " + str(len(reps[i][1])) +
                       ", " + str(reps[i][1].count('0')) + ", " + str(reps[i][1].count('1')))

        for j in range(0, len(offset_counts)):
            out_file.write(", " + str(offset_counts[j]))

        out_file.write("\n")


if __name__ == "__main__":
    argc = len(sys.argv)

    if argc != 6:
        print("Arguments: traceFileStart, traceFileStride, traceFileCount, traceBaseName, outFileBaseName") 
        exit(-1)

    x, tr_start, tr_stride, tr_count, in_base, out_base = sys.argv 
    tr_start = int(tr_start)
    tr_stride = int(tr_stride)
    tr_count = int(tr_count)

    for i in range(0, tr_count):
        access_num = tr_start + i * tr_stride
        footprint_offset_stats(in_base + str(access_num), out_base + str(access_num), 6, 4)
      