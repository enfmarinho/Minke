# Minke
Working in progress UCI compatible chess engine done in order to learn and to get 
fun. Based on NNUE evaluation, Bitboard board representation and a lot of search 
enhancements.

# Compiling and running
There is a CMakeList.txt file in order to facilitate the compiling process, so
you just need to clone this repository, then use the cmake command to generate 
the build directory and then trigger the compilation process, for that, use the
following commands:
```bash
git clone https://github.com/enfmarinho/Minke.git && cd Minke
cmake -S . -B build
cmake --build build
```
This will trigger the compilation process. Then to run it, just use:
```bash
./build/minke
```
# Features
## Board Representation
- Bitboards
- Plain Magic Bitboards
## Move Generation
- Staged Move Generator
### Move ordering
- Transposition Table Move
- Promotions
- Captures 
    - Static Exchange Evaluation (SEE)
    - Most Valuable Victim - Least Valuable Aggressor (MVV-LVA)
    - Capture history
- Historic Heuristic
- Killer Moves
## Search
- Iterative Deepening
- Negamax search with alpha-beta pruning
- Principal Variation Search (PVS)
- Transposition Table Cutoffs
- Late Move Reduction (LMR)
- Null Move Pruning (NMP)
- Reverse Futility Pruning (RFP)
- Quiescence Search
- Mate Distance Pruning
## Evaluation
- Quantized NNUE (Easily Updatable Neural Network)
    - Accumulators
    - SCrelu as the activation neuron
    - Net trainer: [bullet](https://github.com/jw1912/bullet)
    - Net architecture: (768 -> 1024)x2 -> 1
