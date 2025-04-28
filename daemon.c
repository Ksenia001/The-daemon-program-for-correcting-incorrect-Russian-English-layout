#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#define PID_FILE "daemon.pid"

Display *display = NULL;
Window root_window;
KeyCode keycode_r;
unsigned int wanted_modifiers = ControlMask | ShiftMask;
unsigned int ignore_modifiers[] = {0, LockMask, Mod2Mask, Mod3Mask};
int num_ignore_modifiers = sizeof(ignore_modifiers) / sizeof(ignore_modifiers[0]);

void signal_handler(int signum) {
    if (signum == SIGTERM) {
        syslog(LOG_INFO, "Демон получил сигнал SIGTERM и завершает работу.");
        if (display) {
            XUngrabKey(display, keycode_r, AnyModifier, root_window);
            XCloseDisplay(display);
        }
        remove(PID_FILE);
        exit(EXIT_SUCCESS);
    }
}

int main(void) {
    pid_t pid, sid;

    // Fork to create daemon
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

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Register signal handler
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Не удалось зарегистрировать обработчик для SIGTERM");
        exit(EXIT_FAILURE);
    }

    // Write PID to file
    FILE *pid_file = fopen(PID_FILE, "w");
    if (pid_file == NULL) {
        syslog(LOG_ERR, "Не удалось открыть файл для записи PID");
        exit(EXIT_FAILURE);
    }
    fprintf(pid_file, "%d\n", getpid());
    fclose(pid_file);

    // Initialize X11
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        syslog(LOG_ERR, "Не удалось открыть дисплей X11.");
        exit(EXIT_FAILURE);
    }

    int screen = DefaultScreen(display);
    root_window = DefaultRootWindow(display);

    // Get KeyCode for 'R'
    keycode_r = XKeysymToKeycode(display, XK_R);
    if (keycode_r == 0) {
        syslog(LOG_ERR, "Не удалось получить KeyCode для XK_R.");
        XCloseDisplay(display);
        exit(EXIT_FAILURE);
    }

    // Grab Ctrl+Shift+R with all ignore modifiers
    for (int i = 0; i < num_ignore_modifiers; ++i) {
        unsigned int modifiers = wanted_modifiers | ignore_modifiers[i];
        XGrabKey(display, keycode_r, modifiers, root_window, False, GrabModeAsync, GrabModeAsync);
    }

    syslog(LOG_INFO, "Демон запущен и перехватывает Ctrl+Shift+R.");

    // Main event loop
    while (1) {
        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
            if (event.type == KeyPress) {
                syslog(LOG_INFO, "Hello world");
            }
        }
        usleep(10000); // Prevent busy-waiting
    }

    // Cleanup (unreachable, but included for completeness)
    XUngrabKey(display, keycode_r, AnyModifier, root_window);
    XCloseDisplay(display);
    remove(PID_FILE);
    exit(EXIT_SUCCESS);
}