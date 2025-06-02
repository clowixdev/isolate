/*

Isolate - containerization utility
Mishenev Nikita 5131001/30002

*/

#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <wait.h>
#include <memory.h>

#include <iostream>

using namespace std;

#define STACKSIZE (1024*1024)
static char cmd_stack[STACKSIZE];

struct params {
    int fd[2];
    char **argv;
};

void display_help() {
    cout << "Usage: isolate [COMMAND]...\n"
            "Run command in isolated space\n\n"
            "\t-h, --help\t\tdisplay this help message and exit\n\n"
            "Examples:\n"
            "\tisolate bash\t\truns bash console in isolated space\n"
            "\tisolate ./test_script\truns your script in isolated space\n";
}

void kill_thread(const char *fmt, ...) {
    va_list params;

    va_start(params, fmt);
    vfprintf(stderr, fmt, params);
    va_end(params);

    exit(1);
}

void parse_args(int argc, char **argv, params *params) {
    if (--argc < 1) {
        display_help();
        exit(0);
    }

    params->argv = ++argv;
}

void await_setup(int pipe) {
    char buf[2];
    if (read(pipe, buf, 2) != 2) {
        kill_thread("Failed to read from pipe: %m\n");
    }
}

int cmd_exec(void *arg) {
    if (prctl(PR_SET_PDEATHSIG, SIGKILL)) {
        kill_thread("cannot PR_SET_PDEATHSIG for child process: %m\n");
    }

    params *params = (struct params *) arg;
    await_setup(params->fd[0]);

    char **argv = params->argv;
    char *cmd = argv[0];
    cout << "Launching <"<< cmd << "> in isolated namespaces..." << endl;
    cout << "__________________________________________________" << endl;

    if (execvp(cmd, argv) == -1) {
        kill_thread("Failed to exec %s: %m\n", cmd);
    }

    return 1;
}

int main(int argc, char **argv) {
    params params = {0, 0};
    parse_args(argc, argv, &params);

    if (pipe(params.fd) < 0) {
        kill_thread("Failed to create pipe: %m");
    }

    int clone_flags = SIGCHLD | CLONE_NEWUTS;
    int cmd_pid = clone(cmd_exec, cmd_stack + STACKSIZE, clone_flags, &params);

    if (cmd_pid < 0) {
        kill_thread("Failed to clone: %m\n");
    }

    int pipe = params.fd[1];

    // namespace setup

    if (write(pipe, "OK", 2) != 2) {
        kill_thread("Failed to write to pipe: %m");
    }

    if (close(pipe)) {
        kill_thread("Failed to close pipe: %m");
    }

    if (waitpid(cmd_pid, NULL, 0) == -1) {
        kill_thread("Failed to wait pid %d: %m\n", cmd_pid);
    }

    return 0;
}