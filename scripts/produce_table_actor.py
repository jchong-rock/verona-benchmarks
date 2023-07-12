import subprocess
import os
import time
import csv
import argparse
import math
from pathlib import Path

map_min = {}

def getopts():
    parser = argparse.ArgumentParser(description='Gen latex from test run.')
    parser.add_argument('-i', default='./output', help='input directory location')
    args = parser.parse_args()
    return args

def round_special(value, limit):
  value_round = round(value, 1)
  if value >= limit:
    return str(int(round(value, 0)))
  else:
    return str(round(value, 1))

def print_entry(benchmark, map, baseline=None):
  if benchmark in map:
    bold = map[benchmark]["mean"] == map_min[benchmark]
    if bold:
      print("\\underline{\\bfseries ", end='', sep='')
    print(round_special(map[benchmark]["mean"], 100), end='')
    if bold:
      print("}", end='')
    print(" & ", round_special(map[benchmark]["err"], 10), sep='', end='')
    if baseline is not None:
      print(" & (", round_special(math.log(map[benchmark]["mean"] / baseline, 10), 10), ") ", end='', sep='')
    print()
  else:
    print("- &", end='')
    if baseline is not None:
      print(" & ", end='')
    print()
  
def process(file, benchmarks):
  if (not os.path.isfile(file)):
    print(f"File {file} does not exist.")
    return dict()

  map = {}
  with open(file, 'r') as f:
    for row in csv.reader(f):
      benchmark, mean, median, err = row
      if benchmark != "benchmark":
        benchmarks.add(benchmark)
        map[benchmark] = {"mean": float(mean), "median": float(median), "err": float(err)} 
        if benchmark not in map_min or map_min[benchmark] > float(mean):
          map_min[benchmark] = float(mean)

  return map

def process2(file):
  with open(file, 'r') as f:
    map = {}
    for row in csv.reader(f):
      tag, benchmark, dump, steal,lifo,pause,unpause,cowns,b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15 = row
      if benchmark != "Tag":
        map[benchmark] = {"behaviours": b1, "cowns": cowns}
    return map

if __name__ == '__main__':
    args = getopts()

    output_directory = args.i

    benchmarks = set()

    map_ba1 = process(output_directory + f"/boc_actor1.csv", benchmarks)
    map_ba8 = process(output_directory + f"/boc_actor8.csv", benchmarks)
    map_p1  = process(output_directory + f"/pony1.csv", benchmarks)
    map_p8  = process(output_directory + f"/pony8.csv", benchmarks)
    map_stats = process2(output_directory + f"/actor_stats.csv")

    benchfilemap = {
      'Banking': 'banking',
      'Big': 'big',
      'Bounded Buffer': 'bndbuffer',
      'Chameneos': 'chameneos',
      'Cigarette Smokers': 'cigsmok',
      'Concurrent Dictionary': 'concdict',
      'Concurrent Sorted Linked-List': 'concsll',
      'Count': 'count',
      'Dining Philosophers': 'philosopher',
      'Fib': 'fib',
      'Filterbank': 'filterbank',
      'Fork-Join Create': 'fjcreate',
      'Fork-Join Throughput': 'fjthrput',
      'Logistic Map Series': 'logmap',
      'Ping Pong': 'pingpong',
      'Quicksort': 'quicksort',
      'Radixsort': 'radixsort',
      'Recursive Matrix Multiplication': 'recmatmul',
      'Sieve of Eratosthenes': 'sieve',
      'Sleeping Barber': 'barber',
      'Thread Ring': 'threadring',
      'Trapezoid': 'trapezoid'
    }

    filebenchmap = {v: k for k, v in benchfilemap.items()}

    loc_pony = {}
    loc_boc = {}

    with open(output_directory + "/cloc_raw.csv", "r") as file:
      reader = csv.reader(file, delimiter=',')

      for row in reader:
        if (len(row) == 5):
          language, filename, blank, comment, code = row
          if len(Path(filename).parts) != 3:
            continue
          paradigm, group, bench = Path(filename).with_suffix('').parts
          # check if filename begins with actors/ or boc/
          if paradigm == "actors":
            # mistake in matching up benchmark names
            if bench == "fjthroughput":
              bench = "fjthrput"
            # check if filename is in filebenchmap
            if bench in filebenchmap:
              # get benchmark name
              benchmark = filebenchmap[bench]
              loc_boc[benchmark] = int(code)

    with open(output_directory + "/cloc_pony_raw.csv", "r") as file:
      reader = csv.reader(file, delimiter=',')

      for row in reader:
        if (len(row) == 5):
          language, filename, blank, comment, code = row
          if len(Path(filename).parts) != 3:
            continue
          paradigm, benchdir, bench = Path(filename).with_suffix('').parts
          # check if filename begins with actors/ or boc/
          if bench in filebenchmap:
            # get benchmark name
            benchmark = filebenchmap[bench]
            loc_pony[benchmark] = int(code)

    assert (loc_boc.keys() == loc_pony.keys())

    for benchmark in sorted(benchmarks):
      if benchmark == "Banking 2PC":
        continue
      if benchmark == "Recursive Matrix Multiplication":
        print("Matrix Mult")
      else:
        if benchmark == "Concurrent Sorted Linked-List":
          print("Concurrent Sorted List")
        else:
          print(benchmark)
      benchmark = benchmark
      print("&")
      print(loc_pony[benchmark])
      print("&")
      print_entry(benchmark, map_p1)
      print("&")
      print_entry(benchmark, map_p8, map_p1[benchmark]["mean"])
      print("&")
      print(loc_boc[benchmark])
      print("&")
      print_entry(benchmark, map_ba1, map_p1[benchmark]["mean"])
      print("&")
      print_entry(benchmark, map_ba8, map_p1[benchmark]["mean"])
      print("&")
      print(map_stats[benchmark]["cowns"])
      print("&")
      print(f"$10^{{{math.log10(int(map_stats[benchmark]['behaviours'])):.1f}}}$")
      print("\\\\")
      