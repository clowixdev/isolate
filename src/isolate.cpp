/*

Isolate - containerization utility
Mishenev Nikita 5131001/30002

*/

#include <stdarg.h>
#include <sys/prctl.h>
#include <wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <syscall.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

#define STACKSIZE       (1024*1024)
#define DEFAULT_UID     (1000)

static char cmd_stack[STACKSIZE];

void setup_mns(const char *);
void prepare_procfs(void);

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

template <typename T, typename...Params>
void kill_thread(T estr, Params&&...params) {
    printf(estr, params...);
    
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

    params *params = (struct params *)arg;
    await_setup(params->fd[0]);

    setup_mns("mini_rootfs");

    if (setgid(0) == -1) {
        kill_thread("Failed to setgid: %m\n");
    }   
    if (setuid(0) == -1) {
        kill_thread("Failed to setuid: %m\n");
    }

    char **argv = params->argv;
    char *cmd = argv[0];
    cout << "Launching <"<< cmd << "> in isolated namespaces...\n"
            "__________________________________________________" << endl;

    if (execvp(cmd, argv) == -1) {
        kill_thread("Failed to exec %s: %m\n", cmd);
    }

    return 1;
}

void clean_ss(stringstream &ss1, stringstream &ss2) {
    ss1.str("");
    ss1.clear();
    ss2.str("");
    ss2.clear();
}

void setup_uns(int pid) {
    stringstream path;
    stringstream line;

    path << "/proc/" << pid << "/uid_map";
    line << "0 " << DEFAULT_UID << " 1\n";
    ofstream uid_mapper(path.str());
    uid_mapper << line.str();
    uid_mapper.close();

    clean_ss(path, line);
    
    path << "/proc/" << pid << "/setgroups";
    line << "deny";
    ofstream setgroups_setup(path.str());
    setgroups_setup << line.str();
    setgroups_setup.close();

    clean_ss(path, line);

    path << "/proc/" << pid << "/gid_map";
    line << "0 " << DEFAULT_UID << " 1\n";
    ofstream gid_mapper(path.str());
    gid_mapper << line.str();
    gid_mapper.close();
}

void setup_mns(const char *rootfs) {
    const char *mnt = rootfs;

    if (mount(rootfs, mnt, "ext4", MS_BIND, "")) {
        kill_thread("Failed to mount %s at %s: %m\n", rootfs, mnt);
    }

    if (chdir(mnt)) {
        kill_thread("Failed to chdir to rootfs mounted at %s: %m\n", mnt);
    }

    const char *old_fs = ".old_fs";

    if (mkdir(old_fs, 0777) && errno != EEXIST) {
        kill_thread("Failed to mkdir old_fs %s: %m\n", old_fs);
    }

    if (syscall(SYS_pivot_root, ".", old_fs)) {
        kill_thread("Failed to pivot_root from %s to %s: %m\n", rootfs, old_fs);
    }

    if (chdir("/")) {
        kill_thread("Failed to chdir to new root: %m\n");
    }

    if (mkdir("/proc", 0555) && errno != EEXIST) {
        kill_thread("Failed to mkdir /proc: %m\n");
    }

    if (mount("proc", "/proc", "proc", 0, "")) {
        kill_thread("Failed to mount proc: %m\n");
    }

    if (umount2(old_fs, MNT_DETACH)) {
        kill_thread("Failed to umount old_fs %s: %m\n", old_fs);
    }
}

int main(int argc, char **argv) {
    params params = {0, 0};
    parse_args(argc, argv, &params);

    if (pipe(params.fd) < 0) {
        kill_thread("Failed to create pipe: %m");
    }

    int clone_flags = SIGCHLD | CLONE_NEWUTS | CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID;
    int cmd_pid = clone(cmd_exec, cmd_stack + STACKSIZE, clone_flags, &params);

    if (cmd_pid < 0) {
        kill_thread("Failed to clone: %m\n");
    }

    int pipe = params.fd[1];

    setup_uns(cmd_pid);

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