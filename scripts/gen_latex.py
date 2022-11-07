import subprocess
import os
import time
import csv
import argparse


map_min = {}

def getopts():
    parser = argparse.ArgumentParser(description='Gen latex from test run.')
    parser.add_argument('-o', default='./output', help='output directory location')
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
      print("{ \\bfseries ")
    print(round_special(map[benchmark]["mean"], 1000))
    print("$\\pm$", round_special(map[benchmark]["err"], 100))
    if bold:
      print("}")
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


if __name__ == '__main__':
    args = getopts()

    output_directory = args.o

    # Dump it as a csv file so we can use it for pgfplots
    with open(output_directory + f"/boc_actor{1}.csv", 'r') as ba1,\
      open(output_directory + f"/boc_actor{4}.csv", 'r') as ba8,\
      open(output_directory + f"/pony{1}.csv", 'r')      as p1,\
      open(output_directory + f"/pony{4}.csv", 'r') as p8:

      rba1 = csv.reader(ba1)
      rba8 = csv.reader(ba8)
      rp1  = csv.reader(p1)
      rp8  = csv.reader(p8)

      benchmarks = set()

      map_ba1 = process(rba1, benchmarks)
      map_ba8 = process(rba8, benchmarks)
      map_p1  = process(rp1, benchmarks)
      map_p8  = process(rp8, benchmarks)

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
        print_entry(benchmark, map_p1)
        print("&")
        print_entry(benchmark, map_p8)
        print("&")
        print_entry(benchmark, map_ba1)
        print("&")
        print_entry(benchmark, map_ba8)
        print("\\\\")
