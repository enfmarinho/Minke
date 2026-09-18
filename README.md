<div align="center">
  <img src="./logo.png" alt="Minke" width="400"/>

[![License][license-badge]][license-link]  
[![Latest release][release-badge]][release-link]
[![Commits][commits-badge]][commits-link]

</div>

## Strength
|     Version     | [CCRL 40/15][CCRL 40/15] | [CCRL Blitz][CCRL Blitz] | [ipman R9][ipman R9]     | [COPE Bullet][COPE Bullet] | [COPE Rapid][COPE Rapid] | [COPE Classical 8CPU][COPE Classical]   |
|:---------------:|:-----------------------:|:-------------------------:|:------------------------:|:--------------------------:|:------------------------:|:---------------------------------------:|
| [7.0.0][v7.0.0] |          3575 (#26-27)  |           3703 (#27)      |          3509 (#25)      |            3677 (#16)      |          3653 (#19)      |          3650 (#17)                     |
| [6.0.0][v6.0.0] |          3503           |           3594            |           -              |             -              |           -              |           -                             |
| [5.0.0][v5.0.0] |          3463           |           3548            |           -              |             -              |           -              |           -                             |
| [4.0.0][v4.0.0] |          3427           |           3485            |           -              |             -              |           -              |           -                             |
| [3.0.0][v3.0.0] |          3376           |           3428            |           -              |             -              |           -              |           -                             |
| [2.0.0][v2.0.0] |          3330           |           3331            |           -              |             -              |           -              |           -                             |
| [1.0.0][v1.0.0] |           -             |            -              |           -              |             -              |           -              |           -                             |


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
- Full-legal
#### Move ordering
- Transposition Table Move
- Noisy 
    - Static Exchange Evaluation (SEE)
    - Noisy history heuristic
    - Promotions
- Quiet
    - Historic Heuristic
    - Continuation History Heuristic
    - Killer Moves
### Search
- Iterative Deepening
- Aspiration Windows Search 
- Negamax with Alpha-Beta Pruning
- Transposition Table Cutoffs
- Cuckoo table
- Principal Variation Search
- Null Move Pruning
- Reverse Futility Pruning
- Futility Pruning
- Razoring
- Prob-cut
- Late Move Reduction
- Late Move Pruning
- Quiet Futility Pruning
- Quiet History Pruning
- SEE Pruning
- Internal Iterative Reduction
- Improving Heuristic
- Singular Extension
- Multi-cut
- Quiescence Search
- Mate Distance Pruning
- Correction History Heuristic
    - Pawn correction
    - White non-pawn Correction
    - Black non-pawn Correction
    - Continuation Correction
### Evaluation
- Quantized NNUE (Easily Updatable Neural Network)
    - Net architecture: (768x16hm->1280pw)x2->(16x2->32->1)x8
    - Trained exclusively on self-generated data from a random initialization

*Note: this list is not exhaustive*

## Acknowledgements
Minke has been inspired by several other chess engines, such as:
* [Alexandria](https://github.com/PGG106/Alexandria)
* [Altair](https://github.com/Alex2262/AltairChessEngine)
* [Berserk](https://github.com/jhonnold/berserk)
* [Ethereal](https://github.com/AndyGrant/Ethereal)
* [Hobbes](https://github.com/kelseyde/hobbes-chess-engine)
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

[CCRL Blitz]: https://computerchess.org.uk/404/cgi/compare_engines.cgi?family=Minke
[CCRL 40/15]: https://computerchess.org.uk/4040/cgi/compare_engines.cgi?family=Minke
[ipman R9]: https://ipmanchess.yolasite.com/r9-7945hx.php
[COPE Classical]: https://cope-chess.live/ratings?rating_list_id=7
[COPE Rapid]: https://cope-chess.live/ratings?rating_list_id=6
[COPE Bullet]: https://cope-chess.live/ratings?rating_list_id=5

[v7.0.0]: https://github.com/enfmarinho/Minke/releases/tag/v7.0.0
[v6.0.0]: https://github.com/enfmarinho/Minke/releases/tag/v6.0.0
[v5.0.0]: https://github.com/enfmarinho/Minke/releases/tag/v5.0.0
[v4.0.0]: https://github.com/enfmarinho/Minke/releases/tag/v4.0.0
[v3.0.0]: https://github.com/enfmarinho/Minke/releases/tag/v3.0.0
[v2.0.0]: https://github.com/enfmarinho/Minke/releases/tag/v2.0.0
[v1.0.0]: https://github.com/enfmarinho/Minke/releases/tag/v1.0.0
