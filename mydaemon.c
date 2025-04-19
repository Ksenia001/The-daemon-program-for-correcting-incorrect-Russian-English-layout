#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>

#define PID_FILE "daemon.pid"

void signal_handler(int signum) {
    if (signum == SIGTERM) {
        syslog(LOG_INFO, "Демон получил сигнал SIGTERM и завершает работу.");
        remove(PID_FILE);
        exit(EXIT_SUCCESS);
    }
}

int main(void) {
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Не удалось зарегистрировать обработчик для SIGTERM");
        exit(EXIT_FAILURE);
    }

    FILE *pid_file = fopen(PID_FILE, "w");
    if (pid_file == NULL) {
        syslog(LOG_ERR, "Не удалось открыть файл для записи PID");
        exit(EXIT_FAILURE);
    }
    fprintf(pid_file, "%d\n", getpid());
    fclose(pid_file);

    while (1) {
        sleep(30);
    }
    exit(EXIT_SUCCESS);
}