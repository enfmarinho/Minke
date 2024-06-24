'''
Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 
Licensed under the MIT License.
See the LICENSE file in the project root for more information. ''' 
'''
The idea of this script is to run your chess engine and check its perft output 
against the perft output of a chess engine that is believed to be correct, such 
as stockfish, which would be the source of truth. Any differences between yours 
and the reference would be printed out so that you could check your movegen
correctness and debug it. 
You must give the path to the reference binary, to your chess engine binary, to 
a file with a list of FEN and the desired perft depth. 
You can run it like:
    python3.11 compare_perft.py stockfish path_to_your_engine fen_list.txt 5
If the command-line arguments are not provided you'll be asked via stdin. '''
import subprocess
import sys
import time

def read_paths():
    argv = sys.argv
    if (len(argv) > 3):
        return argv[1], argv[2], argv[3]

    return input("Path to reference: "), input("Path to yours: "), input("Path to fen dataset: ")

def read_perft_depth():
    argv = sys.argv
    if len(argv) > 4:
        return argv[4]

    return input("Perft depth: ")

def read_dataset(dataset_path):
    try:
        with open(dataset_path, 'r') as file:
            lines = file.readlines()
        return [line.strip() for line in lines] 
    except Exception as e:
        print(f"Could not open dataset file: {e}")
        return []

def open_pipe(pipe_path):
    return subprocess.Popen(
        [pipe_path], 
        stdin=subprocess.PIPE, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE, 
        text=True
    )

def fead_pipe(pipe, fen, depth):
    pipe.stdin.write(f"position fen {fen}\n")
    pipe.stdin.write(f"go perft {depth}\n")
    pipe.stdin.flush()

    data = {}
    nodes_searched = 0
    while True:
        line = pipe.stdout.readline()
        if line == "\n":
            continue

        words = line.split()
        if words[0] == "Nodes":
            nodes_searched = int(words[2])
            break
        elif (words[0] != "info"):
            data[words[0][:-1]] = int(words[1])

    return data, nodes_searched

def ignore_first_line(pipe):
    pipe.stdout.readline()

def close_pipe(pipe):
    pipe.stdin.close()
    pipe.stdout.close()
    pipe.stderr.close()
    pipe.wait()

def compare(ref_data, yours_data):
    equal = True
    for key in ref_data:
        if key not in yours_data:
            print(f"{key} was searched by ref, but not by yours")
            equal = False

    for key in yours_data:
        if key not in ref_data:
            print(f"{key} was searched by yours, but not by ref")
            equal = False

    for key in yours_data:
        if key in ref_data and ref_data[key] != yours_data[key]:
            print(f"for {key} ref searched {ref_data[key]} nodes, but yours searched {yours_data[key]}")
            equal = False

    return equal

def main():
    ref_path, yours_path, dataset_path = read_paths()
    perft_depth = read_perft_depth()

    ref_pipe = open_pipe(ref_path)
    yours_pipe = open_pipe(yours_path)
    fen_dataset = read_dataset(dataset_path)

    # Ignore chess engine info
    ignore_first_line(ref_pipe)
    ignore_first_line(yours_pipe)

    start_time = time.time()
    for fen in fen_dataset:
        print(f"Starting perft for FEN: {fen}")
        ref_data, ref_nodes_searched = fead_pipe(ref_pipe, fen, perft_depth)
        yours_data, yours_nodes_searched = fead_pipe(yours_pipe, fen, perft_depth)
        equal = compare(ref_data, yours_data)
        if ref_nodes_searched != yours_nodes_searched:
            print("reference searched {ref_nodes_searched} nodes, but yours searh")
            equal = False
        if not equal:
            print("Finishing test earlier due to perft failure")
            break

    print(f"Finished test in {round(time.time() - start_time)} seconds")

    close_pipe(ref_pipe)
    close_pipe(yours_pipe)

if __name__ == "__main__":
    main()
