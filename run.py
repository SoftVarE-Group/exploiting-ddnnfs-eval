from src.performancebenchmark import run_benchmark
from os import listdir
from os.path import isfile, join
import os
import sys

def run_benchmark_file(path):
    if os.path.isdir(path):
        for file in listdir(path):
            file_path = "{}/{}".format(path, file)
            run_benchmark_file(file_path)
    else:
        print(f"Executing benchmark for run configuration: {path}")
        run_benchmark(path)
        print("\n")

if __name__ == "__main__":
    if len(sys.argv) == 2:
        run_benchmark_file(sys.argv[1])
    else:
        print("Wrong number of arguments")
