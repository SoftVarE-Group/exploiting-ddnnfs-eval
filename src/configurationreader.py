import yaml
import json


def parse_file(path):
    if path.endswith(".yaml"):
        with open(path, mode='r') as yaml_file:
            content_class = yaml.safe_load(yaml_file)
            return content_class
    elif path.endswith(".json"):
        with open(path, mode='r') as json_file:
            content_class = json.load(json_file)
            return content_class
    else:
        raise Exception('File format \"' + path + '\" is not supported! Only .yaml and .json files can be used.')
