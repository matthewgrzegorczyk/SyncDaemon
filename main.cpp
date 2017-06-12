#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <utime.h>

#define DAEMON_SLEEP_TIME_S 300 // 5 minutes


// Function declarations.
int daemonRunner();

int syncFolders(char *folderA, char *folderB);

bool copyFile(char *lpszSrcFullFilePath, char *lpszDstFullFilePath);

bool testFolders(char *dirA, char *dirB);

int logInfo(char *text);

int logInfo(char *text, int log_level);

void signal_handler(int signum);


// Global variables.
char szCurrentDir[FILENAME_MAX];

char szDirPathA[FILENAME_MAX];
char szDirPathB[FILENAME_MAX];

volatile int iDaemonSleepTimeS = DAEMON_SLEEP_TIME_S;
volatile bool recursive = false;

int main(int argc, char *argv[]) {

    // Display run parameters.
    printf("Parameters: ");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");


    for (int i = 0; i < argc; i++) {
        if (argc == 1 || (strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            printf("\n\nSyncDaemon help:\n");
            printf("Program syntax: ./SyncDaemon source_dir destination_dir [-r] [-t 10]\n");
            printf("-r - recursive mode\n");
            printf("-t time (time in seconds)\n");

            return EXIT_SUCCESS; // Early exit.
        } else if (strcmp(argv[i], "-r") == 0) {
            printf("Recursive\n");
            recursive = true;
        } else if (strcmp(argv[i], "-t") == 0) {
            i++;
            iDaemonSleepTimeS = atoi(argv[i]);
            printf("Sleep time: %d\n", iDaemonSleepTimeS);
        }
    }

    printf("\n");

    if (argc >= 3) {
        strcpy(szDirPathA, argv[1]);
        strcpy(szDirPathB, argv[2]);
    }

    getcwd(szCurrentDir, sizeof(char) * FILENAME_MAX);
    printf("CWD: %s\n", szCurrentDir);

    if (!testFolders(szDirPathA, szDirPathB)) {
        printf("Jeden z folderów nie istnieje.\n");
        exit(EXIT_FAILURE);
    }

/* Our process ID and Session ID */
    pid_t pid, sid;

/* Fork off the parent process */
    pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (

            getpid()

            != pid) {
        sleep(1);
    }
/* If we got a good PID, then
we can exit the parent process. */
    if (pid > 0) {
        printf("Main proccess id %d\n",

               getpid()

        );
        printf("Fork process id %d\n", pid);
        printf("Proces główny kończy swoje działanie, daemon został poprawnie zainicjalizowany.");
        exit(EXIT_SUCCESS);
    }

/* Change the file mode mask */
    umask(0);

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
    signal(SIGUSR1, signal_handler);

/* Daemon loop */
    while (1) {
        int status = daemonRunner();
        sleep(iDaemonSleepTimeS);
    }
    exit(EXIT_SUCCESS);
}

void signal_handler(int signum) {
    if (signum == SIGUSR1) {
        logInfo("Otrzymano sygnał SIGUSR1. Daemon zostaje wybudzony.");
    }
}

bool testFolders(char *dirA, char *dirB) {
    bool foldersExists;
    DIR *pDirA = opendir(dirA);
    DIR *pDirB = opendir(dirB);

    foldersExists = pDirA != NULL && pDirB != NULL;

    closedir(pDirA);
    closedir(pDirB);

    return foldersExists;
}

int daemonRunner() {
    logInfo("Daemon rozpoczyna swoją pracę.");
    if (chdir(szCurrentDir) < 0) {
        return EXIT_FAILURE;
    }

    syncFolders(szDirPathA, szDirPathB);
//    syncFolders(szDirPathB, szDirPathA);

    logInfo("Daemon zakończył synchronizowanie katalogów i przechodzi w stan uśpienia.");
    return EXIT_SUCCESS;
}

int syncFolders(char *folderA, char *folderB) {
    DIR *pDirA = opendir(folderA);
    DIR *pDirB = opendir(folderB);
    struct dirent *pDirent;
    struct stat fileAttrInDirA;
    struct stat fileAttrInDirB;
    char szFileInfo[FILENAME_MAX];
    char szPathNameA[FILENAME_MAX];
    char szPathNameB[FILENAME_MAX];
    char szTimeBuf[200];
    bool bStatus;


    while ((pDirent = readdir(pDirA)) != NULL) {
        // skip . and ..
        if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0) {
            continue;
        }
        // prepare full path
        sprintf(szPathNameA, "%s/%s", folderA, pDirent->d_name);
        sprintf(szPathNameB, "%s/%s", folderB, pDirent->d_name);

        stat(szPathNameA, &fileAttrInDirA);

        // determine if source path is a file or directory
        if (S_ISREG(fileAttrInDirA.st_mode)) {

            // check if destination file exists
            if (access(szPathNameB, 0) == 0) {

                stat(szPathNameB, &fileAttrInDirB);

                if (fileAttrInDirA.st_mtime > fileAttrInDirB.st_mtime) {
                    bStatus = copyFile(szPathNameA, szPathNameB);
                }
                else {
                    continue;
                }

                sprintf(szFileInfo, "Synchronized %s %s (mod time)\n", szPathNameA, bStatus ? "done" : "failed");
                logInfo(szFileInfo);
            } else {
                bStatus = copyFile(szPathNameA, szPathNameB);
                sprintf(szFileInfo, "Synchronized %s %s (not exists)\n", szPathNameA, bStatus ? "done" : "failed");
                logInfo(szFileInfo);
            }
        } else if (recursive) {
            // check if directory exists
            DIR *pDir = opendir(szPathNameB);
            if (pDir) {
                closedir(pDir);
            } else {
                mkdir(szPathNameB, 0777);
            }
            syncFolders(szPathNameA, szPathNameB);
        }
    }

    closedir(pDirA);
    closedir(pDirB);

    return EXIT_SUCCESS;
}

bool copyFile(char *lpszSrcFullFilePath, char *lpszDstFullFilePath) {
    int fdSrc, fdDst;

    fdSrc = open(lpszSrcFullFilePath, O_RDONLY);
    if (fdSrc < 0) {
        return false;
    }
    fdDst = open(lpszDstFullFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fdDst < 0) {
        close(fdSrc);
        return false;
    }
    ssize_t nRead, nWritten;
    char buf[4096];
    char *pBufOff;

    // copy file content
    while (nRead = read(fdSrc, buf, sizeof buf), nRead > 0) {
        pBufOff = buf;
        do {
            nWritten = write(fdDst, pBufOff, nRead);
            if (nWritten >= 0) {
                nRead -= nWritten;
                pBufOff += nWritten;
            }
        } while (nRead > 0);
    }
    close(fdDst);
    close(fdSrc);

    // change creation and modification times
    struct stat attr;
    struct utimbuf newTm;

    stat(lpszSrcFullFilePath, &attr);
    newTm.actime = attr.st_atime;
    newTm.modtime = attr.st_mtime;
    utime(lpszDstFullFilePath, &newTm);

    return true;
}

int logInfo(char *text) {
    logInfo(text, LOG_INFO);
}

int logInfo(char *text, int log_level) {
    openlog("SyncDaemon", LOG_PID | LOG_CONS, LOG_USER);
    syslog(log_level, text);
    closelog();

    return EXIT_SUCCESS;
}