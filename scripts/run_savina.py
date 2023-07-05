import subprocess
import os
import time
import csv
import argparse

# This will measure setup and teardown time


def getopts():
    parser = argparse.ArgumentParser(description='Run throughput test.')
    parser.add_argument('--repeats', type=int, default=30,
                        help='number of times to repeat the runs')
    parser.add_argument('-o', default='output', help='output directory location')
    args = parser.parse_args()
    return args

def run_test(args, filename):
    with open(filename, 'w') as file:
        # set stdout to file
        subprocess.run(args, check=True, stdout=file, stderr=subprocess.STDOUT)

def run_boc_actor(cores, output_directory, repeats):
    print(f"Running boc (actor) on {cores} core")
    # run the boc actor benchmark
    run_test(["./savina", "--actor", "--csv", "--cores", f'{cores}', "--reps", f'{repeats}'], output_directory + f"/boc_actor{cores}.csv")

def run_boc_full(cores, output_directory, repeats):
    print(f"Running boc (full) on {cores} core")
    run_test(["./savina", "--full", "--csv", "--cores", f'{cores}', "--reps", f'{repeats}'], output_directory + f"/boc_full{cores}.csv")

def run_pony(cores, output_directory, repeats):
    print(f"Running Pony on {cores} core")
    # run the pony benchmark
    run_test(["./savina-pony", "--parseable", "--ponymaxthreads", f"{cores}", "--reps", f'{repeats}'], output_directory + f"/pony{cores}.csv")

if __name__ == '__main__':
    args = getopts()

    output_directory = args.o
    if not os.path.exists(output_directory):
        os.mkdir(output_directory)

    run_test(["./savina-stats", "--cores", "1", "--reps", "1", "--actor"], output_directory + f"/actor_stats.csv")
    run_test(["./savina-stats", "--cores", "1", "--reps", "1", "--full"], output_directory + f"/boc_stats.csv")

    run_test(["./savina", "--scale", "--csv", "--cores", "72", "--reps", "50"], output_directory + f"/banking_scale.csv")

    run_pony(8, output_directory, args.repeats)
#    run_pony(4, output_directory, args.repeats)
#    run_pony(2, output_directory, args.repeats)
    run_pony(1, output_directory, args.repeats)

    run_boc_full(1, output_directory, args.repeats)
#    run_boc_full(2, output_directory, args.repeats)
#    run_boc_full(4, output_directory, args.repeats)
    run_boc_full(8, output_directory, args.repeats)

    run_boc_actor(1, output_directory, args.repeats)
#    run_boc_actor(2, output_directory, args.repeats)
#    run_boc_actor(4, output_directory, args.repeats)
    run_boc_actor(8, output_directory, args.repeats)
