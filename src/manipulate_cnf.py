import os
import subprocess
import time
import timeit
from src.memory_management import limit_virtual_memory


def card_of_config_cril(file_path, config_path, command_array, max_memory, timeout, output_pipe_string, setup):
    """the solver from needs all the input into stdin and therefore we have to messure that time seperatly"""
    try:
        result = 0
        # create the input for stdin the reasoner from cril needs
        if file_path.endswith(".nnf"):
            std_input = f"load {file_path}\n"
        else:
            std_input = f"load {file_path}.nnf\n"
        for c in get_configs_from_file(config_path):  # remove prefix "-out=" and postfix ".nnf"
            std_input += "mc"
            for j in c:
                std_input += f" {j}"
            std_input += "\n"
        # subprocess.PIPE is used to suppress the output of the command call
        t = timeit.Timer(
            stmt="subprocess.run({}, preexec_fn=limit_virtual_memory({}), timeout={}{}, stderr = subprocess.PIPE, input={})".format(
                command_array[0], max_memory, timeout, output_pipe_string, str.encode(std_input)), setup=setup)

        result += t.timeit(number=1)[0]
        if result > timeout:
            print(str(command_array[0]) + ' timed out after {} seconds'.format(timeout))
            return timeout
    except subprocess.TimeoutExpired as expired:
        # if the first call (or any after that) itself exceeds the time limit
        print(expired)
        return timeout
    return result


def card_of_config_sharp_sat(file_path, config_path, command_array, max_memory, timeout, output_pipe_string, setup):
    """cardinality of partial configurations for #SAT solver requires us to change the cnf to include/exclude the features of each partial configuration"""
    try:
        # subprocess.PIPE is used to suppress the output of the command call
        result = 0
        for config in get_configs_from_file(config_path):
            # print("Computing the cardinality of partial configurations for the configuration {}...".format(config))
            add_lines(file_path, config)
            t = timeit.Timer(
                stmt="subprocess.run({}, preexec_fn=limit_virtual_memory({}), timeout={}{}, stderr = subprocess.PIPE)".format(
                    command_array[0], max_memory, timeout, output_pipe_string), setup=setup)
            result += t.timeit(number=1)[0]  # add up all the results
            remove_lines(file_path, len(config))

            if result > timeout:
                print(str(command_array[0]) + ' timed out after {} seconds'.format(timeout))
                return timeout
    except subprocess.TimeoutExpired as expired:
        # if the first call (or any after that) itself exceeds the time limit
        print(expired)
        remove_lines(file_path, len(config))
        return timeout
    return result


def get_configs_from_file(file_path):
    """opens the config file and extracts all partial configurations as a list of ints"""
    with open(file_path, mode='r', encoding="utf-8") as f:
        config = []
        for line in f.readlines():
            config.append([int(n) for n in line.split()])
        return config


def card_of_features_cril(file_path, cnf_path, command_array, max_memory, timeout, output_pipe_string, setup):
    """the solver from needs all the input into stdin and therefore we have to messure that time seperatly"""
    try:
        result = 0
        # create the input for stdin the reasoner from cril needs
        number_of_features = get_number_of_features(cnf_path)
        std_input = f"load {file_path}\n"
        for f in range(number_of_features):
            std_input += f"mc {f}\n"
        std_input += "q\n"

        # subprocess.PIPE is used to suppress the output of the command call
        t = timeit.Timer(
            stmt="subprocess.run({}, preexec_fn=limit_virtual_memory({}), timeout={}{}, stderr = subprocess.PIPE, input={})".format(
                command_array[0], max_memory, timeout, output_pipe_string, str.encode(std_input)), setup=setup)
        result += t.timeit(number=1)[0]
        if result > timeout:
            print(str(command_array[0]) + ' timed out after {} seconds'.format(timeout))
            return timeout
    except subprocess.TimeoutExpired as expired:
        # if the first call (or any after that) itself exceeds the time limit
        print(expired)
        return timeout
    return result


def card_of_features_sharp_sat(file_path, command_array, max_memory, timeout, output_pipe_string, setup):
    """cardinality of features for #SAT solver requires us to change the cnf to include each feature"""
    try:
        # subprocess.PIPE is used to suppress the output of the command call
        result = 0
        # create every cnf for cardinality of features
        features = get_number_of_features(file_path)
        for f in range(1, features):
            # print("Computing the cardinality of feature for feature number {}...".format(f))
            add_lines(file_path, [f])
            t = timeit.Timer(
                stmt="subprocess.run({}, preexec_fn=limit_virtual_memory({}), timeout={}{}, stderr = subprocess.PIPE)".format(
                    command_array[0], max_memory, timeout, output_pipe_string), setup=setup)
            result += t.timeit(number=1)[0]  # add up all the results
            remove_lines(file_path, 1)

            if result > timeout:
                print(str(command_array[0]) + ' timed out after {} seconds and completed {}% of the features'.format(timeout, (f/features)*100.0))
                return timeout
    except subprocess.TimeoutExpired as expired:
        # if the first call (or any after that) itself exceeds the time limit
        print(expired)
        remove_lines(file_path, 1)
        return timeout
    return result


def get_number_of_features(file_path):
    """searches for the header line of the form "p cnf c v" with c as the number of features"""
    with open(file_path, mode='r', encoding="utf-8") as f:
        for line in f.readlines():
            if line.startswith('p cnf'):
                return int(line.split(' ')[2])
    raise Exception('File with the path ' + str(file_path) + ' does not contain a line with the form "p cnf v c" with v as the number of variables.')


def add_lines(file_path, features):
    """adds features for card of features and card of partial configruations"""
    with open(file_path, mode='a+', encoding="utf-8") as f:
        # include a feature with "v 0\n" with v as the number corresponding to the feature
        # exclude with "-" before the feature number
        f.seek(f.tell()-1, os.SEEK_SET)
        if f.read(1) == "\n":  # if there is an empty line at the end of the file
            for feature in features:
                f.write("{} 0\n".format(feature))
        else:
            f.write("\n") # no empty line
            for feature in features:
                f.write("{} 0\n".format(feature))
    adjust_header(file_path, len(features))


def remove_lines(file_path, amount):
    """removes as many lines of the end of a file as mentioned in amount"""
    with open(file_path, mode="a+", encoding="utf-8") as f:
        for _ in range(amount):
            # Move the pointer (similar to a cursor in a text editor) to the end of the file
            f.seek(0, os.SEEK_END)

            # This code means the following code skips the very last character in the file -
            # i.e. in the case the last line is null we delete the last line
            # and the penultimate one
            pos = f.tell() - 1

            # Read each character in the file one at a time from the penultimate
            # character going backwards, searching for a newline character
            # If we find a new line, exit the search
            while pos > 0 and f.read(1) != "\n":
                pos -= 1
                f.seek(pos, os.SEEK_SET)

            # So long as we're not at the start of the file, delete all the characters ahead
            # of this position
            if pos > 0:
                f.seek(pos, os.SEEK_SET)
                f.truncate()

            # add a new line at the end of the file
            f.seek(0, os.SEEK_END)
            f.write("\n")
    adjust_header(file_path, -amount)


def adjust_header(file_path, change):
    """searches for the header line of the form "p cnf c v" with v as the number of lines in the cnf and adjust that number"""
    reading_file = open(file_path, "r")
    new_file_content = ""
    for line in reading_file:
        # change the header
        if line.startswith('p cnf'):
            c = int(line.split(' ')[2])
            v = int(line.split(' ')[3])
            line_changed = 'p cnf {} {}'.format(c, v+change)
            new_file_content += line_changed + "\n"
        # keep the rest the same
        else:
            new_file_content += line
    reading_file.close()

    writing_file = open(file_path, "w")
    writing_file.write(new_file_content)
    writing_file.close()
