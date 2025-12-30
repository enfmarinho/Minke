<div align="center">
  <img src="./logo.png" alt="Minke" width="400"/>

[![License][license-badge]][license-link]  
[![Latest release][release-badge]][release-link]
[![Commits][commits-badge]][commits-link]

</div>

## Building
### Clone the Repository
```
git clone https://github.com/enfmarinho/Minke.git
cd Minke
```
### Compile
To compile Minke use the makefile like so: 
```
make EXE=minke <ARCH>
```

Replace \<ARCH\> with the desired target architecture:
- avx2
- bmi2
- avx512
- native
- apple-silicon

This will generate the executable called minke

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
    - Capture history
- Killer Moves
- Counter moves
- Historic Heuristic
### Search
- Iterative Deepening
- Negamax with Alpha-Beta Pruning
- Aspiration Windows Search 
- Transposition Table Cutoffs
- Principal Variation Search
- Null Move Pruning
- Reverse Futility Pruning
- Futility Pruning
- Razoring
- Late Move Reduction
- Late Move Pruning
- Internal Iterative Reduction
- Improving Heuristic
- Singular Extension
- Multi-cut
- Quiescence Search
- Mate Distance Pruning
### Evaluation
- Quantized NNUE (Easily Updatable Neural Network)
    - SCrelu as the activation neuron
    - Net architecture: (768 -> 1024)x2 -> 1
    - Trained with a mix of Lella and Stockfish data

## Acknowledgements
Minke has been inspired by several other chess engines, such as:
* [Alexandria](https://github.com/PGG106/Alexandria)
* [Altair](https://github.com/Alex2262/AltairChessEngine)
* [Berserk](https://github.com/jhonnold/berserk)
* [Ethereal](https://github.com/AndyGrant/Ethereal)
* [Stockfish](https://github.com/official-stockfish/Stockfish)
* [Stormphrax](https://github.com/Ciekce/Stormphrax)
  
Other important tools used in the development of Minke include:
* [bullet](https://github.com/jw1912/bullet)
* [CPW](https://www.chessprogramming.org/Main_Page)
* [fastchess](https://github.com/Disservin/fastchess)
* [OpenBench](https://github.com/AndyGrant/OpenBench)


[license-link]: https://github.com/enfmarinho/Minke/blob/main/LICENSE
[release-link]: https://github.com/enfmarinho/Minke/releases/latest
[commits-link]: https://github.com/enfmarinho/Minke/commits/main

[license-badge]: https://img.shields.io/github/license/enfmarinho/Minke?style=for-the-badge
[release-badge]: https://img.shields.io/github/v/release/enfmarinho/Minke?style=for-the-badge
[commits-badge]: https://img.shields.io/github/commits-since/enfmarinho/Minke/latest?style=for-the-badge
