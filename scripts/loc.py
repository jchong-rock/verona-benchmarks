import subprocess
import os
import time
import csv
import argparse

# This will measure setup and teardown time


def getopts():
    parser = argparse.ArgumentParser(description='Calculate lines of code.')
    parser.add_argument('-o', default='cloc.csv', help='output file')
    args = parser.parse_args()
    return args

def run_cloc():
    subprocess.run(["cloc", '.', '--match-d="actors|boc"', "--csv", "--by-file", "--quiet", "--out=cloc_raw.csv"])

if __name__ == '__main__':
    args = getopts()

    benchfilemap = {
      'Banking': 'concurrency/banking.h',
      'Chameneos': 'micro/chameneos.h',
      'Count': 'micro/count.h',
      'Dining Philosophers': 'concurrency/philosophers.h',
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
    loc_boc = {}

    with open("cloc_raw.csv", "r") as file:    
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

