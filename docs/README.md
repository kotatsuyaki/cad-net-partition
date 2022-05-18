<!-- vim: set ft=markdown.pandoc colorcolumn=100: -->

# Build and Run

This project can be built with GNU Make.
Compiler supporting C++17 or higher is required.

```sh
make CXX=g++ -j
```

Alternatively, use CMake to build.

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

To run the binary, run

```sh
./pa2 $INPUTFILE $OUTPUTFILE
```

.
See `src/config.hpp` for compile-time and environment variable options.

# Algorithm and Data Structure

Simulated Annealing (SA) is used in this project to solve multiple-way hypergraph partitioning problem.

## Temperature Scheduling

- Temperature always starts at $1.0$ and ends at $0.05$.
- Total running time is fixed as $105$ minutes.
- After each move, the temperature $T$ is multiplied by a factor $f$.
  The factor is periodically updated based on number of moves per second and remaining time,
  in such a way that the temperature gradually drops to $0.05$ at the right time.
  In particular, the factor is calculated by $f = \sqrt[rv]{0.05/T}$, where $r$ is the remaining time,
  and where $v$ is the number of moves per second.

## Moving Cells and Bookkeeping Cost

In each pass, a cell is sampled uniformly from all of the cells to be moved,
to a destination block sampled uniformly from all of the blocks.
To avoid iterating over all of the nets and blocks to calculate the new cost after each move,
Two kinds of extra information are kept.

- $\beta(N_i, B_j) =$ the number of cells in net $N_i$ that is currently in block $B_j$.
- $\sigma(N_i) =$ the number of blocks spanned by net $N_i$.

Upon moving a cell from block $B_j$ to $B_k$, every net $N_i$ connected to the cell is checked.

- If $\beta(N_i, B_j) = 1$, then after the move it becomes zero.  Thus, $\sigma(N_i)$ decreases by $1$. 
- If $\beta(N_i, B_k) = 0$, then after the move it becomes non-zero.  Thus, $\sigma(N_i)$ increases by $1$. 

The $\sigma$ values before and after the move are then used to update the cost.

## Starting Partition

The starting partition is found by repeatedly increasing $k$ and trying to fit the cells inside the $k$ blocks.
For each cell, it assigns the cell to the block with minimum current area, breaking ties by sampling uniformly at random.
