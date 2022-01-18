def build_command(command_with_placeholders, parameter_dictionary):
    return replace_placeholders(command_with_placeholders, parameter_dictionary)


# replaces placeholders {<placeholder>} with value from dict {<placeholder>: <value>}
def replace_placeholders(strings, replace_dictionary):
    res = []
    for string in strings:
        res.append(string.format(**replace_dictionary))
    return res


def create_parameter_dictionary(parameters, default_parameters):
    if parameters is None:
        return default_parameters
    if default_parameters is None:
        return parameters
    para_dict = parameters.copy()
    para_dict.update(default_parameters)
    return para_dict
