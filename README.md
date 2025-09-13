 # Minke
Work in progress UCI compatible chess engine done in order to learn and to get 
fun. Based on NNUE evaluation, Bitboard board representation and a lot of search 
enhancements.

## Building
### Clone the Repository
```
git clone https://github.com/enfmarinho/Minke.git
cd Minke
```
### Compile
To compile Minke use the makefile like so: 
```
make <ARCH>
```

Replace \<ARCH\> with the desired target architecture:
- avx2
- bmi2
- avx512
- native
- apple-silicon

This will generate the executable called minke-\<ARCH\>

## Features
### Board Representation
- Bitboards
- Plain Magic Bitboards
### Move Generation
- Staged Move Generator
- Pseudo-legal
#### Move ordering
- Transposition Table Move
- Promotions
- Captures 
    - Static Exchange Evaluation (SEE)
    - Most Valuable Victim - Least Valuable Aggressor (MVV-LVA)
    - Capture history
- Historic Heuristic
- Killer Moves
### Search
- Iterative Deepening
- Negamax with Alpha-Beta Pruning
- Aspiration Windows Search 
- Principal Variation Search (PVS)
- Transposition Table Cutoffs
- Late Move Reduction (LMR)
- Null Move Pruning (NMP)
- Reverse Futility Pruning (RFP)
- Internal Iterative Reduction (IIR)
- Quiescence Search
- Mate Distance Pruning
- Improving Heuristic
### Evaluation
- Quantized NNUE (Easily Updatable Neural Network)
    - SCrelu as the activation neuron
    - Net architecture: (768 -> 1024)x2 -> 1

## Acknowledgements
Minke has been inspired by several other chess engines, such as:
* [Alexandria](https://github.com/PGG106/Alexandria)
* [Altair](https://github.com/Alex2262/AltairChessEngine)
* [Stormphrax](https://github.com/Ciekce/Stormphrax)
* [Berserk](https://github.com/jhonnold/berserk)
* [Ethereal](https://github.com/AndyGrant/Ethereal)
  
Other important tools used in the development of Minke include:
* [bullet](https://github.com/jw1912/bullet)
* [CPW](https://www.chessprogramming.org/Main_Page)
* [fastchess](https://github.com/Disservin/fastchess)
