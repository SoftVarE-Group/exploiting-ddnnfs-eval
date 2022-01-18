#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "parser.hpp"
#include "prompt.hpp"

using namespace std;

// True if the given option exists, whether it is associated with a value or not.
bool cmdOptionExists(int argc, char* argv[], const string& option) {
	char** end = argv + argc;
	return find(argv, end, option) != end;
}

// Finds the value associated with given option and returns it.
// Returns nullptr if the option is not found or if it has no associated value.
char* getCmdOption(int argc, char* argv[], const string& option) {
	char** end = argv + argc;
    char** res = find(argv, end, option);
    if (res != end && res + 1 != end) {
        return *(res + 1);
    }
    return nullptr;
}

static void displayUsage() {
  cout << "Usage: query-dnnf [-cmd cmd-file]" << endl;
  cout << "Manipulate d-DNNF formulae." << endl;
  cout << endl;
  cout << "  -cmd cmd-file: file from which commands are to be read (stdin as default)" << endl;
  cout << endl;
  cout << "If no file is provided, commands are read from standard input. Commands are:" << endl << endl;
  printHelp();
}

static int manageOptions(int argc, char **argv) {

  if(cmdOptionExists(argc, argv, "--help")) {
    displayUsage();
    return 0;
  }
  if(cmdOptionExists(argc, argv, "-cmd")) {
    char *path = getCmdOption(argc, argv, "-cmd");
    int fdPath = open(path, O_RDONLY);
    if(fdPath == -1) {
      perror("Error while opening command file");
      return 1;
    }
    close(0);
    dup(fdPath);
  }
  return -1;
}

int main(int argc, char* argv[]) {
  int argsStatus = manageOptions(argc, argv);
  if(argsStatus != -1) return argsStatus;
  try {
    prompt();
  } catch (ParserException npe) {
    cerr << "Error while parsing: " << npe.getMessage() << endl;
    return 1;
  } catch (GraphException ge) {
    cerr << "Fatal error: " << ge.getMessage() << endl;
    return 1;
  }
}
