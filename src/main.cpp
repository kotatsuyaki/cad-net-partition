#include <cassert>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "dbg.h"

using std::unordered_set;
using std::vector;
using Net = unordered_set<size_t>;

int main(int argc, char** argv) {
    std::ifstream infile(argv[1]);

    size_t max_group_area;
    size_t ncells, nnets;

    std::string ignore_word;
    size_t ignore_num;

    infile >> max_group_area;
    infile >> ignore_word;
    assert(ignore_word == ".cell");
    infile >> ncells;

    vector<size_t> areas(ncells, 0);
    for (size_t i = 0; i < ncells; i += 1) {
        size_t index;
        infile >> index;
        infile >> areas[index];
    }

    dbg(areas);

    infile >> ignore_word;
    assert(ignore_word == ".net");

    infile >> nnets;
    vector<Net> nets(nnets, Net());

    // `i` is the index of net
    for (size_t i = 0; i < nnets; i += 1) {
        size_t ncells_contained;
        infile >> ncells_contained;
        for (size_t j = 0; j < ncells_contained; j += 1) {
            size_t cell;
            infile >> cell;
            nets[i].insert(cell);
        }
    }

    dbg(nets);

    return 0;
}
