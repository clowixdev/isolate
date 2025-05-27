/*

Isolate - containerization utility
Mishenev Nikita 5131001/30002

*/

#include <iostream>
#include <vector>

using namespace std;

#define ERROR_CODE  -1

void display_help() {
    cout << "Usage: isolate [COMMAND]..." << endl;
    cout << "Run command in isolated space\n" << endl;
    cout << "\t-h, --help\t\t\tdisplay this help message and exit\n" << endl;
    cout << "Examples:" << endl;
    cout << "\tisolate bash\t\truns bash console in isolated space" << endl;
    cout << "\tisolate ./test_script\truns your script in isolated space" << endl;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        display_help();
        return ERROR_CODE;
    }

    string command = argv[1];
    vector<string> arguments;

    if (argc >= 3) {
        for (int i = 2; i < argc; i++) {
            arguments.push_back(argv[i]);
        }
    }

    cout << "c: " << command << endl;
    for (auto arg : arguments) {
        cout << "arg: " << arg << endl;
    }
}