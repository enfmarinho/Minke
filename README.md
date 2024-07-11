# Minke
Working in progress UCI compatible chess engine done in order to learn and to get 
fun. Based on NNUE evaluation, mailbox(0x80) board representation and a lot of search 
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
## Board representation
- Mailbox (0x80)
## Search
- Iterative deepening
- Negamax search with alpha-beta pruning
- Aspiration Windows
- PVS (Principal Variation Search)
- Transposition table cutoffs
### Move ordering
- Transposition Table Move
- Promotions
- Captures 
    - Static Exchange Evaluation (SEE)
    - Most valuable victim - least valuable aggressor (MVV-LVA)
- Historic heuristic
## Evaluation
- Quantized NNUE (easily updatable neural network)
    - Accumulators
    - SCrelu as the activation neuron
    - Net trainer: [bullet](https://github.com/jw1912/bullet)
    - Net architecture: (768 -> 768)x2 -> 1
