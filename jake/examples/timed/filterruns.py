import statistics
import os

def filter_lines(input_file):
    with open(input_file, 'r') as infile:
        boc_out = []
        act_out = []
        boc_count = 10000
        act_count = 10000
        
        for line in infile:
            if line.startswith("boc:"):
                if boc_count > 0:
                    v = line.removeprefix("boc:").lstrip()
                    boc_out.append(int(v))
                    boc_count -= 1
            
            elif line.startswith("act:"):
                if act_count > 0:
                    v = line.removeprefix("act:").lstrip()
                    act_out.append(int(v))
                    act_count -= 1

        print("\n")
        print("ACT MEAN: ", statistics.mean(act_out))
        print("ACT STDV: ", statistics.stdev(act_out))
        print("\n")
        print("BOC MEAN: ", statistics.mean(boc_out))
        print("BOC STDV: ", statistics.stdev(boc_out))
        print("\n")

if __name__ == "__main__":
    filter_lines(os.path.dirname(os.path.abspath(__file__))+"/runs.txt")