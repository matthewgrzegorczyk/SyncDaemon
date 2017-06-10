#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <utime.h>
#include <time.h>

#define DAEMON_SLEEP_TIME 5
#define FILE_PATH "log.txt"

int fd;
int failures = 0;
int counter = 0;

int daemonRunner();

int syncFolders(char *folderA, char *folderB);

bool testFolders(char *dirA, char *dirB);

int logInfo(char *text);

int logInfo(char *text, int log_level);

void handler(int signum);

char *dirA;
char *dirB;
char *currentDir;

int main(int argc, char *argv[]) {
    printf("Initializing.\n");
    printf("Parametry: %s %s %s\n", argv[0], argv[1], argv[2]);

    dirA = argv[1];
    dirB = argv[2];
    currentDir = new char[FILENAME_MAX];
    getcwd(currentDir, sizeof(char) * FILENAME_MAX);
    printf("CWD: %s\n", currentDir);


    if (!testFolders(dirA, dirB)) {
        printf("Jeden z folderów nie istnieje.");
        exit(EXIT_FAILURE);
    }

    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (getpid() != pid) {
        sleep(1);
    }
    /* If we got a good PID, then
    we can exit the parent process. */
    if (pid > 0) {
        printf("Main proccess id %d\n", getpid());
        printf("Fork process id %d\n", pid);

        delete[] currentDir;
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }



    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Daemon-specific initialization goes here */
    signal(SIGUSR1, handler);
    logInfo("Log za sygnałem.");

    /* The Big Loop */
    while (1) {
        /* Do some task here ... */
        int status = daemonRunner();
        sleep(DAEMON_SLEEP_TIME);
    }
    delete[] currentDir;
    exit(EXIT_SUCCESS);
}

void handler(int signum) {
    logInfo("Signal incoming fucker");
}

bool testFolders(char *dirA, char *dirB) {
    bool foldersExists;
    DIR *pDirA = opendir(dirA);
    DIR *pDirB = opendir(dirB);

    foldersExists = pDirA != NULL && pDirB != NULL;

    closedir(pDirA);
    closedir(pDirB);
    pDirA = 0;
    pDirB = 0;

    return foldersExists;
}

int daemonRunner() {

//    int fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0666);
//    // Failed opening the file.
//    if (fd < 0) {
//        failures++;
//        perror("open");
//        char *errorMessage = strerror(errno);
//        logInfo(errorMessage);
//        return EXIT_FAILURE;
//    }
    logInfo("daemonRunner");
    if (chdir(currentDir) < 0) {
        return EXIT_FAILURE;
    }

    syncFolders(dirA, dirB);

    return EXIT_SUCCESS;
}

int syncFolders(char *folderA, char *folderB) {
    DIR *pDirA = opendir(folderA);
    DIR *pDirB = opendir(folderB);
    struct dirent *pDirent;

    while ((pDirent = readdir(pDirA)) != NULL) {
        char test[FILENAME_MAX];

        utimbuf *file_times;
        utime(pDirent->d_name, file_times);

        char buff[20];

        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&file_times->modtime));
//        sprintf(test, "[%s] [%s]\n", pDirent->d_name, buff);
        logInfo(test);
    }

    closedir(pDirA);
    closedir(pDirB);


    return EXIT_SUCCESS;
}

int logInfo(char *text) {
    logInfo(text, LOG_INFO);
}

int logInfo(char *text, int log_level) {
    openlog("slog", LOG_PID | LOG_CONS, LOG_USER);
    syslog(log_level, text);
    closelog();

    return EXIT_SUCCESS;
}