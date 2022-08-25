# Computes time required for a command line process

from src.configurationreader import parse_file
from src.commandparser import build_command, create_parameter_dictionary
from src.outputparser import create_output_line, create_raw_output
import src.configurationcreator as cc
from src.memory_management import limit_virtual_memory
from enum import Enum
import src.constant as const

from itertools import zip_longest
import src.manipulate_cnf as cnf
import timeit
import time
import subprocess
import os
import sys

timeit.template = """
def inner(_it, _timer{init}):
    {setup}
    _t0 = _timer()
    for _i in _it:
        retval = {stmt}
    _t1 = _timer()
    return _t1 - _t0, retval
"""


class Operation(Enum):
    FM = 'FM' # cardinality of feature model
    FS = 'FS' # cardinality of features
    PC = 'PC' # cardinality of partial configurations
    CO = 'CO' # compiler
    PR = 'PR' # preprocessing


def run_benchmark(config_file_path):
    configuration = parse_file(config_file_path)
    create_folder_if_not_exists(configuration[const.OUTPUT_DIR])

    # if we want to compute partial configurations we have to delete the existing ones and create new configs
    if Operation[configuration[const.OPERATION]] is Operation.PC:
        if configuration[const.SHOW_PROGRESS]:
            print("---------------- Creating config files: " +
                  configuration[const.CONFIG_INFORMATION] + " with seed " + str(configuration[const.SEED]) + " ----------------")
        subprocess.run(["rm", "-R", "config/"])
        cc.create_config_files("cnf", configuration[const.SEED], list(eval(configuration[const.CONFIG_INFORMATION])))

    for calls in configuration[const.CMD_CALLS]:
        cmd_file, raw_file = create_files(configuration[const.OUTPUT_DIR], calls[const.NAME])
        if configuration[const.SHOW_PROGRESS]:
            print("---------------- Evaluating: " +
                  calls[const.NAME] + "----------------")
        for instance in calls[const.INSTANCES]:
            handle_instance(instance, calls, configuration, cmd_file, raw_file)
        cmd_file.close()
        raw_file.close()


def handle_instance(instance, calls, configuration, cmd_file, raw_file):
    parameter_dictionary = []
    runtimes = []
    default_dict = instance[const.DEFAULT_PARAMETERS]


    start_folder = 0
    if 'start_folder' in configuration:
        start_folder = configuration[const.START_FOLDER]

    for parameters, repeating_parameters in zip_longest(instance[const.PARAMETERS],
                                                        configuration[const.REPEATING_PARAMETERS]):

        parameter_dictionary = create_parameter_dictionary(parameters, repeating_parameters)
        parameter_dictionary = create_parameter_dictionary(parameter_dictionary, default_dict)
        command = build_command(calls[const.COMMANDS], parameter_dictionary)

        if configuration[const.SHOW_PROGRESS]:
            print("Evaluating: " + ';\n\t    '.join(command) + "...")
        runtimes = []

        for i in range(configuration[const.NUMBER_OF_RUNS]):
            runtimes.append(measure_runtime(command, configuration[const.MAX_MEMORY], configuration[const.TIMEOUT],
                                            configuration[const.SUPPRESS_OUTPUT], configuration[const.OPERATION],
                                            calls[const.NAME], start_folder + i))

        cmd_file.write(create_output_line(parameter_dictionary, calls[const.PRINT_PARAMETERS], runtimes))
        raw_file.write(create_raw_output(parameter_dictionary, calls[const.PRINT_PARAMETERS], runtimes))
    return runtimes, parameter_dictionary


def create_files(output_dir, command):
    file_path = os.path.join(output_dir, command + ".output")
    raw_filepath = os.path.join(output_dir, command + "_raw.output")
    if os.path.isfile(file_path) or os.path.isfile(raw_filepath):
        sys.exit("File already exists.")

    return open(file_path, "w+"), open(raw_filepath, "w+")


def measure_runtime(commands, max_memory, timeout, suppress_output, operation, name, folder):
    setup = ("import subprocess, os\n"
             "from src.memory_management import limit_virtual_memory")
    output_pipe_string = ", stdout=subprocess.PIPE" if suppress_output else ""

    command_array = []
    for command in commands:
        command_array.append(command.split(" "))

    if Operation[operation] is Operation.FM or Operation[operation] is Operation.CO:
        if name.startswith("ddnnife"):
            if command_array[0][-1] == '-o':
                command_array[0].append(f'{cnf.get_number_of_features(command_array[1][0])}')
                del command_array[1]
        try:
            # subprocess.PIPE is used to suppress the output of the command call
            t = timeit.Timer(
                stmt="subprocess.run({}, preexec_fn=limit_virtual_memory({}), timeout={}{}, stderr = subprocess.PIPE)".format(
                    command_array[0], max_memory, timeout, output_pipe_string), setup=setup)
            result = t.timeit(number=1)
        except subprocess.TimeoutExpired as expired:
            print(expired)
            return timeout
        if result[0] > timeout:
            print(';\n\t    '.join(command_array) + 'reached timeout')
            return timeout
        while len(command_array) > 1:
            del command_array[0]

            # for c2d we have to create an individual folder for each ddnnf because we don't want to overwrite
            # previous results due to the non-deterministic behavior of c2d
            if name.startswith("c2d"):
                command_array[0][-1] = command_array[0][-1].replace("c2d/",f"c2d/{folder}/")
                if not os.path.exists(os.path.dirname(command_array[0][-1])):
                    os.makedirs(os.path.dirname(command_array[0][-1]))
            subprocess.run(command_array[0])
        return result[0]
    elif Operation[operation] is Operation.FS:
        if 'c2d' in name:
            for i in range(0, len(command_array[0])):
                command_array[0][i] = command_array[0][i].replace("c2d/",f"c2d/{folder}/")

        if name.startswith("ddnnife"):
            if command_array[0][-1] == '-o':
                command_array[0].append(f'{cnf.get_number_of_features(command_array[1][0])}')
                del command_array[1]
            try:
                # subprocess.PIPE is used to suppress the output of the command call
                t = timeit.Timer(
                    stmt="[subprocess.run(c, preexec_fn=limit_virtual_memory({}), timeout={}{}, stderr = subprocess.PIPE) for c in {}]".format(
                        max_memory, timeout, output_pipe_string, command_array), setup=setup)
                result = t.timeit(number=1)
            except subprocess.TimeoutExpired as expired:
                print(expired)
                return timeout
            if result[0] > timeout:
                print(';\n\t    '.join(command_array) + 'reached timeout')
                return timeout
            return result[0]
        elif name.startswith("query-dnnf"):
            return cnf.card_of_features_cril(command_array[0][1], command_array[0][2], command_array, max_memory, timeout, output_pipe_string, setup)
        elif name.startswith("d4"):
            return cnf.card_of_features_d4(command_array[0][2], command_array, max_memory, timeout, output_pipe_string, setup)
        else:
            return cnf.card_of_features_sharp_sat(command_array[0][-1], command_array, max_memory, timeout, output_pipe_string, setup)
    elif Operation[operation] is Operation.PC:
        if 'c2d' in name:
            for i in range(0, len(command_array[0])):
                command_array[0][i] = command_array[0][i].replace("c2d/",f"c2d/{folder}/")

        if name.startswith("ddnnife"):
            if command_array[0][-1] == '-o':
                command_array[0].append(f'{cnf.get_number_of_features(command_array[1][0])}')
                del command_array[1]
            try:
                # subprocess.PIPE is used to suppress the output of the command call
                t = timeit.Timer(
                    stmt="[subprocess.run(c, preexec_fn=limit_virtual_memory({}), timeout={}{}, stderr = subprocess.PIPE) for c in {}]".format(
                        max_memory, timeout, output_pipe_string, command_array), setup=setup)
                result = t.timeit(number=1)
            except subprocess.TimeoutExpired as expired:
                print(expired)
                return timeout
            if result[0] > timeout:
                print(';\n\t    '.join(command_array) + 'reached timeout')
                return timeout
            return result[0]
        elif name.startswith("query-dnnf"):
            return cnf.card_of_config_cril(command_array[0][1], command_array[0][2], command_array, max_memory, timeout, output_pipe_string, setup)
        elif name.startswith("d4"):
            return cnf.card_of_config_d4(command_array[0][2], command_array[0][4], command_array, max_memory, timeout, output_pipe_string, setup)
        else:
            file_path = command_array[0][-1]
            config_path = command_array[0][-1].replace("cnf", "config").replace("dimacs", "config")
            return cnf.card_of_config_sharp_sat(file_path, config_path, command_array, max_memory, timeout, output_pipe_string, setup)
    elif Operation[operation] is Operation.PR:
        try:
            # subprocess.PIPE is used to suppress the output of the command call
            t = timeit.Timer(
                stmt="[subprocess.run(c, preexec_fn=limit_virtual_memory({}), timeout={}{}, stderr = subprocess.PIPE) for c in {}]".format(
                    max_memory, timeout, output_pipe_string, command_array), setup=setup)
            result = t.timeit(number=1)
        except subprocess.TimeoutExpired as expired:
            print(expired)
            return timeout
        if result[0] > timeout:
            print(';\n\t    '.join(command_array) + 'reached timeout')
            return timeout
        return result[0]
    else:
        raise Exception('There was no operation selected in the experiment file or the selected operation does not exist.')


def create_folder_if_not_exists(name):
    try:
        os.makedirs(name)
    except FileExistsError:
        pass
