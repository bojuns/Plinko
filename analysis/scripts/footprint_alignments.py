from suffix_tree.tree import Tree
from suffix_tree.builder_factory import BUILDERS
import re
import sys
import copy

def maximal_repeats(tree, min_len=2, min_count=2, trim_zeros=True):
    a = []
    for cv, path in sorted(tree.maximal_repeats()):
        if len(path) >= min_len:
            max_rep = str(path).replace(' ', '')
            if re.search('^[01]*1[01]*1[01]*$', max_rep): # all matches should be only 0s and 1s, and at least two 1s
                # strip leading and trailing 0s
                if trim_zeros:
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


def sort_by_num_ones(e):
    return e[0] * e[1].count('1')


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

    return big_str, entries[0][0], len(lines)-1


def get_repeated_footprints(trace_str, base, min_substring_len=8, min_occ=4, footprint_len=64, sort_kernel=sort_by_num_ones, trim_zeros=True):
    tree = Tree({"A": trace_str}, builder=BUILDERS[2])
    reps = maximal_repeats(tree, min_substring_len, min_occ, trim_zeros=trim_zeros)
    reps.sort(key=sort_by_num_ones, reverse=True) # sort repeated substrings by total number of ones (freq*ones_count)
    return reps, tree


def get_footprint_offset_stats(trace_str, base, repeated_footprints, suffix_tree):
    reps = repeated_footprints
    stats = []

    # for each substring...
    for i in range(0, len(reps)):
        occ = suffix_tree.find_all(reps[i][1]) # find all occurrences of substring
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

        stats.append({"substring": str(reps[i][1]), "frequency": reps[i][0], 
                      "length": len(reps[i][1]), "zeros": reps[i][1].count('0'), 
                      "ones": reps[i][1].count('1'), "offsets": offset_counts})

    return stats


# Occurrences of footprints may overlap with footprints of longer length. If a footprint is 
# a substring of a larger footprint, and over 'thresh' fraction of footprints' occurrences
# coincide with the larger footprint, it is considered a duplicate and removed from the list. 
# This implementation has O(n^2) complexity because... well, priorities. :)
def remove_duplicates(stats, thresh=0.98):
    new_stats = copy.deepcopy(stats)

    i = 0
    while i < len(new_stats):
        j = i + 1

        while j < len(new_stats):
            si = new_stats[i]["substring"]
            sj = new_stats[j]["substring"]

            if si in sj or sj in si:
                fi = new_stats[i]["frequency"]
                fj = new_stats[j]["frequency"]

                if len(si) < len(sj) and fj / fi > thresh:
                    new_stats.pop(i)
                    i -= 1 # net effect is that i stays the same for next outer loop iteration
                    break
                elif len(sj) < len(si) and fi / fj > thresh:
                    new_stats.pop(j)
                    continue # net effect is that j stays the same for next inner loop iteration

            j += 1

        i += 1
    
    return new_stats


def write_footprint_offset_stats(stats, outFileName):
    # write CSV data headers
    out_file = open(outFileName, "w")
    out_file.write("Frequency, Substring, Length, Zeroes, Ones")

    for i in range(0, footprint_len):
        out_file.write(", " + str(i)) # write offset headers (CSV format)
    out_file.write("\n")

    for i in range(0, len(stats)):
        # write line in CSV format
        out_file.write(str(stats[i]["frequency"]) + ", \'" + str(stats[i]["substring"]) + "\', " 
                       + str(stats[i]["length"]) + ", " + str(stats[i]["zeros"]) + ", " 
                       + str(stats[i]["ones"]))

        for j in range(0, len(stats[i]["offsets"])):
            out_file.write(", " + str(stats[i]["offsets"][j]))

        out_file.write("\n")
    
    out_file.close()


# must use return value of get_footprint_offset_stats(...) as input
def analyze_footprint_offset_stats(stats, outFileName):
    max_indices = []
    ones_not_mode = []
    ones_total = []

    outFile = open(outFileName, "w")
    
    # for each substring found...
    for i in range(0, len(stats)):
        max_val = stats[i]["offsets"][0]
        max_index = 0

        for j in range(1, len(stats[i]["offsets"])):
            if stats[i]["offsets"][j] > max_val:
                max_val = stats[i]["offsets"][j]
                max_index = j

        max_indices.append(max_index)
        ones_total.append(stats[i]["ones"] * stats[i]["frequency"])
        ones_not_mode.append(stats[i]["ones"] * (stats[i]["frequency"] - stats[i]["offsets"][max_index]))

        outFile.write(f"{ones_total[i]}, {ones_not_mode[i]}, \"=B{i+1}/A{i+1}\"\n")
        #print(f'{stats[i]}: {ones_not_mode[i]} / {ones_total[i]} = {ones_not_mode[i] / ones_total[i] * 100}%')

    outFile.close()

    

if __name__ == "__main__":
    footprint_len = 64
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
        trace_str, base, total_footprints = parse_pht_file(in_base + str(access_num))
        reps, tree = get_repeated_footprints(trace_str, base, 6, 4, footprint_len)
        print("Repeated footprints found.")

        stats1 = get_footprint_offset_stats(trace_str, base, reps, tree)
        #write_footprint_offset_stats(stats1, out_base + str(access_num))
        print("Offset stats calculated.")

        stats2 = remove_duplicates(stats1, 0.90)
        print(f"{len(stats1) - len(stats2)} duplicates removed.")

        write_footprint_offset_stats(stats2, out_base + str(access_num))
        print(f"Wrote stats to file '{out_base + str(access_num)}'.")

        #analyze_footprint_offset_stats(stats1, "test1.csv")
        analyze_footprint_offset_stats(stats2, out_base + '_ones-non-mode' + str(access_num))
        print(f"Wrote ones analysis to file '{out_base + '_ones-non-mode' + str(access_num)}'.")

        # full duplicate footprints
        reps_full, tree_full = get_repeated_footprints(trace_str, base, footprint_len, 2, footprint_len, trim_zeros=False)

        total_repeats = 0

        for j in range(len(reps_full)):
            total_repeats += reps_full[j][0]

        full_footprint_str = f"Total ones count: {trace_str.count('1')}, total footprint count: {total_footprints}, of which {total_repeats} are repeats.\n"

        print(full_footprint_str)
        outFile = open(out_base + "_full-footprints" + str(access_num), "w")
        outFile.write(full_footprint_str)
        outFile.close()
