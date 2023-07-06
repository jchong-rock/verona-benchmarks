import subprocess
import os
import time
import csv
import argparse


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

def print_entry(benchmark, map):
  if benchmark in map:
    bold = map[benchmark]["mean"] == map_min[benchmark]
    if bold:
      print("\\underline{ \\bfseries ")
    print(round_special(map[benchmark]["mean"], 1000))
    if bold:
      print("}")
    print("&", round_special(map[benchmark]["err"], 100))
  else:
    print("-")

def process(file, benchmarks):
  map = {}

  for row in file:
    benchmark, mean, median, err = row
    if benchmark != "benchmark":
      benchmarks.add(benchmark)
      map[benchmark] = {"mean": float(mean), "median": float(median), "err": float(err)} 
      if benchmark not in map_min or map_min[benchmark] > float(mean):
        map_min[benchmark] = float(mean)

  return map

def process2(file):
  map = {}
  for row in file:
    tag, benchmark, dump, steal,lifo,pause,unpause,cowns,b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15 = row
    if benchmark != "Tag":
      map[benchmark] = {"behaviours1": b1, "behaviours2": b2, "cowns": cowns}
  return map

if __name__ == '__main__':
    args = getopts()

    output_directory = args.i

def run_cloc():
    # Not currently working so manually run cloc:
    # cloc . --match-d="actors|boc" --by-file --quiet --csv --out=cloc_raw.csv   
    # subprocess.run(["cloc", ".", "--match-d=actors|boc", "--by-file", "--quiet", "--csv", "--out=cloc_raw.csv"])
    1

if __name__ == '__main__':
    args = getopts()

    benchfilemap = {
      'Banking': 'concurrency/banking.h',
      'Chameneos': 'micro/chameneos.h',
      'Count': 'micro/count.h',
      'Dining Philosophers': 'concurrency/philosopher.h',
      'Fib': 'micro/fib.h',
      'Fork-Join Create': 'micro/fjcreate.h',
      'Fork-Join Throughput': 'micro/fjthroughput.h',
      'Logistic Map Series': 'concurrency/logmap.h',
      'Quicksort' : 'parallel/quicksort.h',
      'Sleeping Barber': 'concurrency/barber.h',
      'Trapezoid': 'parallel/trapezoid.h'}

    filebenchmap = {v: k for k, v in benchfilemap.items()}

    run_cloc()
    loc_actor = {}
    loc_full = {}

    with open(output_directory + "/cloc_raw.csv", "r") as file:    
      reader = csv.reader(file, delimiter=',')

      for row in reader:
        if (len(row) == 5):
          language, filename, blank, comment, code = row
          # check if filename begins with actors/ or boc/
          if filename.startswith("./actors/"):
            # strip actors/ from filename
            filename = filename[9:]
            # check if filename is in filebenchmap
            if filename in filebenchmap:
              # get benchmark name
              benchmark = filebenchmap[filename]
              # print benchmark, code
              loc_actor[benchmark] = int(code)
          if filename.startswith("./boc/"):
            # strip actors/ from filename
            filename = filename[6:]
            # check if filename is in filebenchmap
            if filename in filebenchmap:
              # get benchmark name
              benchmark = filebenchmap[filename]
              # print benchmark, code
              loc_full[benchmark] = int(code)

    # Dump it as a csv file so we can use it for pgfplots
    with open(output_directory + f"/boc_actor1.csv", 'r') as ba1,\
      open(output_directory + f"/boc_actor8.csv", 'r') as ba8,\
      open(output_directory + f"/boc_full1.csv", 'r') as bf1,\
      open(output_directory + f"/boc_full8.csv", 'r')  as bf8,\
      open(output_directory + f"/actor_stats.csv", 'r') as a_stats,\
      open(output_directory + f"/boc_stats.csv", 'r') as b_stats:
      
      rba1 = csv.reader(ba1)
      rba8 = csv.reader(ba8)
      rbf1 = csv.reader(bf1)
      rbf8 = csv.reader(bf8)
      ra_stats  = csv.reader(a_stats)
      rb_stats  = csv.reader(b_stats)

      benchmarks = set()

      map_ba1 = process(rba1, benchmarks)
      map_ba8 = process(rba8, benchmarks)
      map_bf1 = process(rbf1, benchmarks)
      map_bf8 = process(rbf8, benchmarks)

      map_a_stats = process2(ra_stats)
      map_b_stats = process2(rb_stats)

      # caclculate set of benchmarks in map_bf1
      benchmarks = {k for k in benchmarks if k in map_bf1 and k != "Concurrent Dictionary"}
#      benchmarks = {"Banking", "Dining Philosophers", "Fib", "Logistic Map Series", "Sleeping Barber"}

      for benchmark in sorted(benchmarks):
        if benchmark == "Recursive Matrix Multiplication":
          print("Matrix Mult")
        else:
          if benchmark == "Concurrent Sorted Linked-List":
            print("Concurrent Sorted List")
          else:
            print(benchmark)
        benchmark = benchmark
        print("&")
        print(loc_actor[benchmark])
        print("&")
        print_entry(benchmark, map_ba1)
        print("&")
        print_entry(benchmark, map_ba8)
        print("&")
        print(map_a_stats[benchmark]["cowns"])
        print("&")
        print(map_a_stats[benchmark]["behaviours1"])
        print("&")
        print(loc_full[benchmark])
        print("&")
        print_entry(benchmark, map_bf1)
        print("&")
        print_entry(benchmark, map_bf8)
        print("&")
        print(map_b_stats[benchmark]["cowns"])
        print("&")
        print(map_b_stats[benchmark]["behaviours1"])
        print("&")
        print(map_b_stats[benchmark]["behaviours2"])
        print("\\\\")
