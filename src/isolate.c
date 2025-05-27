/*

Isolate - containerization utility
Mishenev Nikita 5131001/30002

*/

#include <stdio.h>

#define ERROR_CODE  -1

void display_help() {
    printf("Usage: isolate [COMMAND]...\n");
    printf("Run command in isolated space\n\n");
    printf("\t\t--help\t\tdisplay this help message and exit\n");
    printf("Examples:\n");
    printf("\tisolate bash\t\truns bash console in isolated space\n");
    printf("\tisolate ./test_script\t\truns your script in isolated space\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        display_help();
        return ERROR_CODE;
    }


}