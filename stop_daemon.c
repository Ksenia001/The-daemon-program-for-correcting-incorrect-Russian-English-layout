#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define PID_FILE "daemon.pid"

int main() {
    FILE *pid_file = fopen(PID_FILE, "r");
    if (!pid_file) {
        fprintf(stderr, "Ошибка открытия файла PID: %s\n", strerror(errno));
        return 1;
    }

    pid_t pid;
    if (fscanf(pid_file, "%d", &pid) != 1) {
        fprintf(stderr, "Ошибка чтения PID из файла\n");
        fclose(pid_file);
        return 1;
    }
    fclose(pid_file);

    if (kill(pid, 0) == -1) {
        if (errno == ESRCH) {
            fprintf(stderr, "Процесс с PID %d не существует\n", pid);
        } else {
            fprintf(stderr, "Ошибка проверки процесса: %s\n", strerror(errno));
        }
        return 1;
    }

    printf("Отправка SIGTERM процессу %d...\n", pid);
    if (kill(pid, SIGTERM) == -1) {
        fprintf(stderr, "Ошибка отправки сигнала: %s\n", strerror(errno));
        return 1;
    }

    sleep(1);
    if (access(PID_FILE, F_OK) == 0) {
        fprintf(stderr, "Предупреждение: PID-файл не был удален\n");
    } else {
        printf("Демон успешно завершен\n");
    }

    return 0;
}