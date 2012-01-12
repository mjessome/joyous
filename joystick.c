#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

#define JOY_DEV  "/dev/input/js0"
#define SHELL    "/bin/sh"
#define BTN_PRESS   0
#define BTN_RELEASE 1

Display *DISPLAY;

typedef struct {
    unsigned int key;
    unsigned int mod;
} Key;
typedef union {
    char *s;
    int i;
    Key k;
} Arg;

typedef struct
{
    unsigned int key;
    void (*func[2])(const Arg *);
    const Arg arg;
} Button;

/* Action Functions */
void print(const Arg *arg)
{
    printf("%s", arg->s);
}
void quit(const Arg *arg)
{
    exit(arg->i);
}
void exec_cmd(const Arg *arg)
{
    if (fork() == 0) {
        fclose(stdout);
        fclose(stderr);
        const char *cmd[] = { SHELL, "-c", arg->s, NULL };
        setsid();
        execvp(((char **)cmd)[0], (char **)cmd);
        exit(0);
    }
}
void send_key_release(const Arg *arg)
{
    if (DISPLAY == NULL) return; /* Error */
    unsigned int keycode = (arg->k.key)? XKeysymToKeycode(DISPLAY, (KeySym)arg->k.key) : 0;
    unsigned int modcode = (arg->k.mod)? XKeysymToKeycode(DISPLAY, (KeySym)arg->k.mod) : 0;

    XTestGrabControl(DISPLAY, True);
    if (keycode) XTestFakeKeyEvent(DISPLAY, keycode, False, CurrentTime);
    if (modcode) XTestFakeKeyEvent(DISPLAY, modcode, False, 0);
    XSync(DISPLAY, False);
    XTestGrabControl(DISPLAY, False);
}
void send_key_press(const Arg *arg)
{
    if (DISPLAY == NULL) return; /* Error */
    unsigned int keycode = XKeysymToKeycode(DISPLAY, (KeySym)arg->k.key);
    unsigned int modcode = (arg->k.mod)? XKeysymToKeycode(DISPLAY, (KeySym)arg->k.mod) : 0;

    XTestGrabControl(DISPLAY, True);
    if (modcode) XTestFakeKeyEvent(DISPLAY, modcode, True, 0);
    if (keycode) XTestFakeKeyEvent(DISPLAY, keycode, True, CurrentTime);
    XSync(DISPLAY, False);
    XTestGrabControl(DISPLAY, False);
}

#include "config.h"

void flush_fd(int fd)
{
    fd_set fds;
    struct timeval t;
    struct js_event js;
    /* set timeout to 0 */
    t.tv_sec = 1;
    t.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    while (select(sizeof(fds)*8, &fds, NULL, NULL, &t) > 0) {
        if (read(fd, &js, sizeof(struct js_event)) < 0) break;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
    }
}

int main(int argc, char *argv[])
{
    int i;
    int debug = 0;
    int joy_fd, x;
    int *axis=NULL;
    int num_of_axis = 0, num_of_buttons=0;
    char *button=NULL, name_of_joystick[80];
    struct js_event js;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d")) {
            debug = 1;
        }
    }

    DISPLAY = XOpenDisplay(NULL);
    if (DISPLAY == NULL) {
        printf("Error: Cannot connect to X server.");
        exit(1);
    }
    if (!XQueryExtension(DISPLAY, "XTEST", &i, &i, &i)) {
        printf("Error: Extension \"XTest\" not available on display.\n");
        exit(1);
    }

    if ((joy_fd = open(JOY_DEV , O_RDONLY)) == -1) {
        printf("Couldn't open joystick\n");
        exit(1);
    }

    ioctl(joy_fd, JSIOCGAXES, &num_of_axis);
    ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);
    ioctl(joy_fd, JSIOCGNAME(80), &name_of_joystick);

    axis = (int *)calloc(num_of_axis, sizeof(int));
    button = (char *)calloc(num_of_buttons, sizeof(char));

    if (debug) {
        printf("Joystick detected: %s\n\t%d axis\n\t%d buttons\n\n",
                name_of_joystick, num_of_axis, num_of_buttons);
    }

    fcntl(joy_fd, F_SETFL, O_RDONLY);
    if (debug) printf("Flushing joystick input...\n");
    flush_fd(joy_fd);
    if (debug) printf("Ready.\n");

    while (1) {
        /* read the joystick state */
        read(joy_fd, &js, sizeof(struct js_event));

        /* see what to do with the event */
        switch (js.type & ~JS_EVENT_INIT)
        {
            case JS_EVENT_AXIS:
                axis[js.number] = js.value;
                break;
            case JS_EVENT_BUTTON:
                button[js.number] = js.value;
                for (i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
                    if (buttons[i].key != js.number+1) continue;
                    if (buttons[i].func[js.value] && buttons[i].key)
                        buttons[i].func[js.value](&buttons[i].arg);
                }
                break;
        }
        if (debug) {
            /* print the results */
            printf("X: %6d  Y: %6d  ", axis[0], axis[1]);
            if (num_of_axis > 2)
                printf("Z: %6d  ", axis[2]);
            if (num_of_axis > 3)
                printf("R: %6d  ", axis[3]);
            for (x=0 ; x<num_of_buttons ; ++x)
                printf("B%d: %d  ", x, button[x]);
            printf("  \r");
            fflush(stdout);
        }
    }

    close(joy_fd);
    return 0;
}
