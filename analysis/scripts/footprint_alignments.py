from suffix_tree.tree import Tree
from suffix_tree.builder_factory import BUILDERS
import re
import sys
import copy

# this function was partially derived from the "../tests/unit/test_maximal_repeats.py" example
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

def sort_stats_by_length(e):
    return len(e['substring'])


# converts PHT trace into an array of footprint strings for processing
def parse_pht_file(path):
    trace = open(path, 'r')
    lines = trace.readlines()
    entries = []

    for i in range(1, len(lines)):
        tok = re.split(" +", lines[i].strip())
        entries.append([int(tok[0]), tok[1]])

    footprints = []
    
    for i in range(2, len(lines)):
        footprints.append(entries[i-1][1])

    return footprints


def get_repeated_footprints(footprints, min_substring_len=8, min_occ=2, sort_kernel=sort_by_num_ones, trim_zeros=True):
    footprints_dict = {}

    for i in range(0, len(footprints)):
        footprints_dict[str(i)] = footprints[i]

    tree = Tree(footprints_dict, builder=BUILDERS[2])
    reps = maximal_repeats(tree, min_substring_len, min_occ, trim_zeros=trim_zeros)
    reps.sort(key=sort_kernel, reverse=True) # sort repeated substrings using specified kernel
    return reps, tree


def get_footprint_offset_stats(footprints, repeated_footprints, suffix_tree):
    reps = repeated_footprints
    stats = []

    # for each substring...
    for i in range(0, len(reps)):
        occ = suffix_tree.find_all(reps[i][1]) # find all occurrences of substring
        offset_counts = [0] * len(footprints[0]) # initialize counter

        # find distribution of footprint offsets for this substring (iterate over all occurrences)
        for j in range(0, len(occ)):
            # index of occurrence is length of trace_str minus length of suffix 
            # (excluding internally-appended '$' character)
            str_offset = len(footprints[0]) - (len(occ[j][1]) - 1)

            # get offset relative to footprint
            footprint_offset = str_offset % len(footprints[0])

            # update respective counter
            offset_counts[footprint_offset] += 1

        stats.append({"substring": str(reps[i][1]), "frequency": reps[i][0], 
                      "length": len(reps[i][1]), "zeros": reps[i][1].count('0'), 
                      "ones": reps[i][1].count('1'), "offsets": offset_counts})

    return stats


def get_num_overlaps(super1, super2, sub, tree):
    pos1 = super1.find(sub)
    pos2 = super2.find(sub)

    #print(f'{super1}, {super2}, {sub}')
    #print(f'pos1={pos1}')
    #print(f'pos2={pos2}')

    if pos1 < 0 or pos1 < 0:
        return 0

    prefix1 = super1[ : pos1 + len(sub)]
    suffix1 = super1[pos1 : ]
    prefix2 = super2[ : pos2 + len(sub)]
    suffix2 = super2[pos2 : ]

    #print(f'{prefix1}, {suffix1}, {prefix2}, {suffix2}')

    big_super = ""

    if prefix1 == suffix2:
        big_super = prefix2 + suffix1[len(sub) : ]
    elif prefix2 == suffix1:
        big_super = prefix1 + suffix2[len(sub) : ]
    else:
        return 0
    
    print(f'big={big_super}')
    
    matches = tree.find_all(big_super)
    return len(matches)


# NOTE: This implementation is currently incomplete and does not handle overlapping substrings properly.
# Occurrences of footprints may overlap with footprints of longer length. If a footprint is 
# a substring of a larger footprint, and over 'thresh' fraction of footprints' occurrences
# coincide with the larger footprint, it is considered a duplicate and removed from the list. 
# NOTE: for this algorithm to work, all duplicates must be present in stats.
def remove_duplicates(stats, tree):
    # deep-copied array must be descending order to properly remove substring instances
    new_stats = copy.deepcopy(stats)
    new_stats.sort(key=sort_stats_by_length, reverse=True) 

    dups_removed = 0

    # for every string pair...
    i = 0
    while i < len(new_stats):
        j = i + 1

        while j < len(new_stats):
            # get substring and frequency of the current string pair
            sl = new_stats[i]["substring"] # large substring
            ss = new_stats[j]["substring"] # small substring
            fl = new_stats[i]["frequency"] # frequency of large substring
            fs = new_stats[j]["frequency"] # frequency of small substring

            # smaller string must be at index i
            assert(len(ss) <= len(sl))
            substring_offset = sl.find(ss)

            if substring_offset >= 0:
                print(f'i={i}, j={j}')
                print(f'{ss} ({fs}) found in {sl} ({fl})')
                total_overlaps = 0
                offset_overlaps = [0] * len(new_stats[j]['offsets'])

                if 0:
                    # for any superstrings already visited, make sure that overlapping instances are not counted more than once
                    for k in range(0, i):
                        overlaps = get_num_overlaps(new_stats[k]["substring"], sl, ss, tree)

                        if overlaps > 0:
                            print(f'{overlaps}, {i}, {j}, {k}')
                            total_overlaps += overlaps
                            
                            for l in range(0, len(new_stats[j]['offsets'])):
                                offset_overlaps[l] += new_stats[k]["offsets"][l]
                    
                    removed = fl - total_overlaps
                    fs -= removed # remove substring instances already represented in superstring
                    dups_removed += removed

                    print(f'Total: {total_overlaps}')
                    print(f'removed {removed}')

                assert(fs >= 0)
                new_stats[j]["frequency"] = fs
                
                if(0): # disable this part during debugging
                    # if no instances are left, remove the entry
                    if fs == 0:
                        new_stats.pop(j)
                        continue # must not update j since new elem moved to its index

                    elif(0): # otherwise, update the counts in the stats array
                        dups_for_this_superstring = 0

                        # remove offset instances from the substring that are already represented in the superstring
                        for k in range(0, len(new_stats[j]['offsets']) - substring_offset):
                            dups_for_this_offset = new_stats[i]['offsets'][k] - offset_overlaps[k]
                            dups_for_this_superstring += dups_for_this_offset
                            new_stats[j]['offsets'][k + substring_offset] -= dups_for_this_offset
                            #print(f"{k}, {k+substring_offset}, {new_stats[j]['offsets'][k + substring_offset]} -= {dups_for_this_offset}")
                            assert(new_stats[j]['offsets'][k + substring_offset] >= 0) # should never be negative

                        assert(dups_for_this_superstring == fl) # total change in offsets total should equal change in frequency field

            j += 1

        i += 1
    
    return new_stats, dups_removed


def write_footprint_offset_stats(stats, outFileName, footprint_len):
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
    ftpt_nonprimary_offset = []
    ftpt_total = []
    ftpt_total_total = 0
    ftpt_nonprimary_offset_total = 0

    outFile = open(outFileName, "w")
    outFile.write("TotalSubfootprints, InNonPrimaryOffset, Fraction\n")
    
    # for each substring found...
    for i in range(0, len(stats)):
        max_val = stats[i]["offsets"][0]
        max_index = 0

        for j in range(1, len(stats[i]["offsets"])):
            if stats[i]["offsets"][j] > max_val:
                max_val = stats[i]["offsets"][j]
                max_index = j

        max_indices.append(max_index)
        ftpt_total.append(stats[i]["frequency"])
        ftpt_total_total += stats[i]["frequency"]
        ftpt_nonprimary_offset.append(stats[i]["frequency"] - stats[i]["offsets"][max_index])
        ftpt_nonprimary_offset_total += stats[i]["frequency"] - stats[i]["offsets"][max_index]

        outFile.write(f"{ftpt_total[i]}, {ftpt_nonprimary_offset[i]}, {ftpt_nonprimary_offset[i]/ftpt_total[i]}\n")
    
    outFile.write("TOTALS, , \n")
    outFile.write(f"{ftpt_total_total}, {ftpt_nonprimary_offset_total}, {ftpt_nonprimary_offset_total/ftpt_total_total}\n")
    outFile.close()


# tests duplicates removal with multiple degrees of overlap between substrings
def test():
    test_footprints = ["1010110011101011001100011100110001"]
    test_footprints = ["11110101011001110101100110001110011000111110101011001110101100110001110011000100001101011001110101100110001110011000100001"]
    test_reps, test_tree = get_repeated_footprints(test_footprints, 6, 2)
    test_stats = get_footprint_offset_stats(test_footprints, test_reps, test_tree)
    print(test_stats)
    test_stats, test_removed = remove_duplicates(test_stats, test_tree)
    print(test_stats)
    print(f"{test_removed} duplicates removed.")


if __name__ == "__main__":
    argc = len(sys.argv)

    if argc != 3:
        print("Arguments: pht_snapshot_name, output_base_name") 
        exit(-1)

    #test()
    #sys.exit()

    x, in_name, out_base = sys.argv 

    footprints = parse_pht_file(in_name)
    reps, tree = get_repeated_footprints(footprints, 6, 2)
    footprint_len = len(footprints[0])
    print("Repeated footprints found.")

    stats = get_footprint_offset_stats(footprints, reps, tree)
    print("Offset stats calculated.")
    #write_footprint_offset_stats(stats, out_base + "_BEFORE_DUPS_REMOVAL", footprint_len)

    #stats, dups_removed = remove_duplicates(stats, tree)
    #print(f"{dups_removed} duplicates removed.")

    write_footprint_offset_stats(stats, out_base, footprint_len)
    print(f"Wrote stats to file '{out_base}'.")

    analyze_footprint_offset_stats(stats, out_base + '_nonprimary-offset')
    print(f"Wrote ones analysis to file '{out_base + '_nonprimary-offset'}'.")

    # full duplicate footprints
    reps_full, tree_full = get_repeated_footprints(footprints, footprint_len, 2, trim_zeros=False)

    total_repeats = 0

    for i in range(len(reps_full)):
        total_repeats += reps_full[i][0] - 1 # only count extra instances

    total_ones = 0

    for i in range(len(footprints)):
        total_ones += footprints[i].count('1')

    full_footprint_str = f"Total ones count: {total_ones}, total footprint count: {len(footprints)}, of which {total_repeats} are repeats.\n"

    print(full_footprint_str)
    outFile = open(out_base + "_full-footprints", "w")
    outFile.write(full_footprint_str)
    outFile.close()
