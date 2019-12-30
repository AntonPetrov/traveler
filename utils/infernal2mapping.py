"""
Reads INFERNAL mapping which contains one line for sequence and one for the structure. The sequence contains
in upper case nucleotides which are mapped from template to target. Lower case letters denote nucleotides
which are inserted into target respective to template and "-" denote positions which were deleted with respect
to template. The structure then contains the corresponding secondary structure, i.e. secondary structure of template
with added .~ characters corresponding to insertions (these will map the lowercase positions). Removing the positions
corresponding to "-" nucleotides will result to target structure while removing ".~" characters results in the
template structure.

Parsing the INFERNAL input results in two structures which contain, sequnece and structure of both target and
template (the template sequence has "-" at positions which were deleted since this information is not present in
the mapping file) and mapping list which gives mapping from target/template to the position in the infernal
mapping (since this contains both insertions and deletions).

The structures are then processed into the pre-order sorted list of nodes of the target and template trees. Each node
contains the INFERNAL-indexed index/indexes of the respective nucleotides from the tree-based structure representation
(i.e. base-pairs represented by inner nodes and non-paired nucleotides by leafs [except of some special cases when
hairpin ends with a base pair]).

The pre-ordered lists are then scanned to identify indexes of nodes in the template tree which need to be matched/inserted/deleted
to arrive to the target tree. This mapping is then sent to output and can be imported by Traveler.
"""
import sys
import argparse
import logging

from typing import List, Tuple


class SequenceStructureMapping:
    def __init__(self):
        self.sq = "" # sequence
        self.str = "" # structure
        self.m = [] # mapping of position in tgt or tmp to the infernal string (which comprises both tgt and tmp information)


def error_exit(message):
    logging.error(message)
    sys.exit()


def read_structures(f) -> List[SequenceStructureMapping]:
    f.readline()
    sq = f.readline()
    f.readline()
    str = f.readline()
    assert len(sq) == len(str)

    tgt_s_s_m = SequenceStructureMapping()
    tmp_s_s_m = SequenceStructureMapping()

    tgt_s_s_m.sq = sq.replace('-', '')
    tmp_s_s_m.str = str.replace('.', '').replace('~', '')

    for i, letter in enumerate(sq):
        if letter != '-':
            tgt_s_s_m.str += str[i]
            tgt_s_s_m.m.append(i)
        if not 'a' <= letter <= 'z':
            tgt_s_s_m.sq += letter
            tmp_s_s_m.m.append(i)

    return [tmp_s_s_m, tgt_s_s_m]

def read_str_ix(s_s_m: SequenceStructureMapping) -> List[List[int]]:
    str_ix = []
    stack_normal = [] #(
    stack_curly = [] #{
    stack_arrow = [] #<
    stack_square = [] #[
    str = s_s_m.str
    for ix in range(len(str)):
        if str[ix] not in '{<([}>)]':
            str_ix.append([s_s_m.m[ix]])
        elif str[ix] in '{<([':
            if str[ix] == '(':
                stack = stack_normal
            elif str[ix] == '{':
                stack = stack_curly
            elif str[ix] == '<':
                stack = stack_arrow
            elif str[ix] == '[':
                stack = stack_square
            stack.append(ix)
        else:
            if str[ix] == ')':
                stack = stack_normal
            elif str[ix] == '}':
                stack = stack_curly
            elif str[ix] == '>':
                stack = stack_arrow
            elif str[ix] == ']':
                stack = stack_square
            ix1 = stack.pop()

            str_ix.append([ix1, ix]) #for the use case of mapping it does not matter in which order the nodes are sorted
            # str_ix = [[ix1, ix]] + str_ix

    return str_ix


def find_in_mapping(ixs, str_ix: List[List[int]]) -> int:
    for i in range(len(str_ix)):
        ixs1 = str_ix[i]
        if len(ixs) == 1:
            if ixs[0] == ixs1[0]:
                return  i
        else:
            if ixs[0] == ixs1[0] and ixs[1] == ixs1[1]:
                return i
    return -1


def get_mapping(tmp_str_ix: List[List[int]], tgt_str_ix: List[List[int]]) -> List[Tuple[int, int]]:
    m = []
    tgt_mapped = []
    for i_tmp in range(len(tmp_str_ix)):
        i_tgt = find_in_mapping(tmp_str_ix[i_tmp], tgt_str_ix)
        m.append((i_tmp+1, i_tgt+1))
        tgt_mapped.append(i_tgt)

    for i_tgt in set(range(len(tgt_str_ix))).difference(tgt_mapped):
        m.append((0, i_tgt + 1))

    return m

def get_distance(m: List[Tuple[int]]) -> int:
    d = 0
    for i1, i2 in m:
        if i1 == 0 or i2 == 0:
            d += 1
    return d


def main():
    with open(args.input, "r") as fr:
        tgt_s_s_m, tmp_s_s_m = read_structures(fr)
        tgt_str_ix = read_str_ix(tgt_s_s_m)
        tmp_str_ix = read_str_ix(tmp_s_s_m)
        m = get_mapping(tmp_str_ix, tgt_str_ix)
        with (sys.stdout if args.output is None else open(args.output, "w")) as fw:
            fw.write('DISTANCE: {}\n'.format(get_distance(m)))
            for i1, i2 in m:
                fw.write('{} {}\n'.format(i1, i2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument("-i", "--input",
                        required=True,
                        metavar='FILE',
                        help="Infernal FASTA-like .afa file with the alignment.")
    parser.add_argument("-o", "--output",
                        metavar='FILE',
                        help="Output file name for the Traveler-formatted mapping. "
                             "If non entered, the standard output will be used.")

    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s [%(levelname)s] %(module)s - %(message)s',
        datefmt='%H:%M:%S')

    main()