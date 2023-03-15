import src.manipulate_cnf as cnf

import numpy.random as npr
import os
import pathlib
from pathlib import Path
from fnmatch import fnmatch
from pysat.formula import CNF
from pysat.solvers import Solver


def create_config_files(root_dir, seed, setup):
    """creates a random file containing partial configurations for each file in the cnf folder"""
    npr.seed(seed)

    # get all .dimacs files and create an adjusting config file
    pattern = "*.dimacs"
    for path, sub_dirs, files in os.walk(root_dir):
        for name in files:
            if fnmatch(name, pattern):
                create_config_file(str(pathlib.PurePath(path, name)), setup, seed)


def create_config_file(file_path, setup, seed):
    config_path = file_path.replace("cnf", "config").replace("dimacs", "config")
    formula = CNF(from_file=file_path)
    number_of_features = cnf.get_number_of_features(file_path)

    s = Solver(bootstrap_with=formula)

    assumptions = set()
    fails = 0
    fail_counter = 0

    config = []

    for (length, number, satisfiable) in setup:
        # if there are not enough features that can be selected without duplicates: skip
        if length >= number_of_features:
            continue
        for i in range(number):
            while len(assumptions) != length:
                # if we took bad random choices and run into a configuration that probably
                # can not be extended to the length we would like to restart the random config
                # to avoid being stuck in a dead end
                if fails > 10000:
                    assumptions.clear()
                    fails = 0

                # there are enough features to create a config with the desired length but
                # there is no valid combination
                if fail_counter > 10:
                    assumptions.clear()
                    fail_counter = 0
                    break

                # add a candidate to the set and keep it if the assumption is still satisfiable
                candidate = npr.randint(-number_of_features, number_of_features)
                if candidate == 0:
                    continue
                assumptions.add(candidate)
                if satisfiable:  # if we want satisfiable configs: remove last candidate if the assumption evaluates to false
                    if not s.solve(assumptions=assumptions):
                        fails += 1
                        assumptions.remove(candidate)
                else:
                    if s.solve(assumptions=assumptions) and len(assumptions) == length:  # we can randomly add variables but at the last we have to make sure that the config is not satisfiable
                        fails += 1
                        assumptions.remove(candidate)

            config.append(" ".join([str(a) for a in list(assumptions)]))
            assumptions.clear()

    # create directory tree according to the cnf tree
    Path(config_path.rsplit('/', 1)[0]).mkdir(parents=True, exist_ok=True)
    with open(config_path, mode='a+', encoding="utf-8") as f:
        for c in config:
            if c != config[-1]:  # the last line should not add an additional \n
                f.write("{}\n".format(c))
            else:
                f.write(c)
