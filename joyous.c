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

typedef struct {
    unsigned int key;
    void (*func[2])(const Arg *);
    const Arg arg;
} Button;

void print(const Arg *arg);
void quit(const Arg *arg);
void exec_cmd(const Arg *arg);
void send_key_release(const Arg *arg);
void send_key_press(const Arg *arg);
void send_string(const Arg *arg);

#include "config.h"

static Key char_to_key(const char *c);
static void flush_fd(int fd);
static void print_status_info(int *axis, int n_axis,
                              char *button, int n_buttons);
static void usage();

/* Action Functions */
void
print(const Arg *arg)
{
    printf("%s", arg->s);
    fflush(stdout);
}
void
quit(const Arg *arg)
{
    exit(arg->i);
}
void
exec_cmd(const Arg *arg)
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
void
send_key_release(const Arg *arg)
{
    if (DISPLAY == NULL) return; /* Error */
    unsigned int keycode =
        (arg->k.key) ? XKeysymToKeycode(DISPLAY, (KeySym)arg->k.key) : 0;
    unsigned int modcode =
        (arg->k.mod) ? XKeysymToKeycode(DISPLAY, (KeySym)arg->k.mod) : 0;

    XTestGrabControl(DISPLAY, True);
    if (keycode) XTestFakeKeyEvent(DISPLAY, keycode, False, CurrentTime);
    if (modcode) XTestFakeKeyEvent(DISPLAY, modcode, False, 0);
    XSync(DISPLAY, False);
    XTestGrabControl(DISPLAY, False);
}
void
send_key_press(const Arg *arg)
{
    if (DISPLAY == NULL) return; /* Error */
    unsigned int keycode = XKeysymToKeycode(DISPLAY, (KeySym)arg->k.key);
    unsigned int modcode =
        (arg->k.mod) ? XKeysymToKeycode(DISPLAY, (KeySym)arg->k.mod) : 0;

    XTestGrabControl(DISPLAY, True);
    if (modcode) XTestFakeKeyEvent(DISPLAY, modcode, True, 0);
    if (keycode) XTestFakeKeyEvent(DISPLAY, keycode, True, CurrentTime);
    XSync(DISPLAY, False);
    XTestGrabControl(DISPLAY, False);
}
void
send_string(const Arg *arg)
{
    int c;
    char *str = (char *)calloc(2, sizeof(char));
    Key k;
    Arg a;

    str[1] = '\0';
    for (c = 0; c < strlen(arg->s); c++) {
        k = char_to_key(&arg->s[c]);
        a.k = k;
        send_key_press(&a);
        send_key_release(&a);
    }
    free(str);
}

static Key
char_to_key(const char *c)
{
    Key k;
    char str[2];
    char x = *c;

    str[0] = x;
    str[1] = '\0';
    k.mod = 0;
    if (x > 0x40 && x < 0x5b) {
        k.mod = XK_Shift_L;
    }
    /* handle special characters */
    switch (x) {
        case '\n':
            k.key = XK_Return;
            break;
        case '\t':
            k.key = XK_Tab;
            break;
        default:
            str[0] = *c;
            k.key = XStringToKeysym(str);
            break;
    }
    return k;
}

static void
flush_fd(int fd)
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

static void
print_status_info(int *axis, int n_axis, char *button, int n_buttons)
{
    int x;
    /* print the results */
    printf("X: %6d  Y: %6d  ", axis[0], axis[1]);
    if (n_axis > 2)
        printf("Z: %6d  ", axis[2]);
    if (n_axis > 3)
        printf("R: %6d  ", axis[3]);
    for (x = 0; x < n_buttons; ++x)
        printf("B%d: %d  ", x, button[x]);
    printf("  \r");
    fflush(stdout);
}

static void
usage()
{
    printf("Usage: joyous [options]\n");
    printf("Options:\n\n  Optional\n");
    printf("    -d,--debug\tPrint debug information to console.\n");
    printf("    -i,--info\tRun in info mode; Won't call any functions, but ");
    printf("    shows joystick information and information on keypresses.\n");
    printf("    -h, --help\tDisplay this help message\n");
}

int
main(int argc, char *argv[])
{
    int i;
    int debug = 0,
        info_mode = 0;
    int joy_fd;
    int num_of_axis = 0;
    int num_of_buttons = 0;
    int *axis = NULL;
    char *button=NULL;
    char name_of_joystick[80];
    struct js_event js;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
            debug = 1;
        }
        else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--info")) {
            info_mode = 1;
            debug = 1;
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage();
            exit(0);
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

    joy_fd = open(JOY_DEV, O_RDONLY);
    if (joy_fd == -1) {
        printf("Couldn't open joystick\n");
        exit(1);
    }

    ioctl(joy_fd, JSIOCGAXES, &num_of_axis);
    ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);
    ioctl(joy_fd, JSIOCGNAME(80), &name_of_joystick);

    axis = (int *)calloc(num_of_axis, sizeof(int));
    button = (char *)calloc(num_of_buttons, sizeof(char));

    if (debug) {
        printf("%s\n\t%d axis\n\t%d buttons\n\n",
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
                if (info_mode) break;
                for (i = 0; i < sizeof(buttons)/sizeof(Button); i++) {
                    if (buttons[i].key != js.number+1) continue;
                    if (buttons[i].func[js.value] && buttons[i].key)
                        buttons[i].func[js.value](&buttons[i].arg);
                }
                break;
        }

        if (debug || info_mode) {
            print_status_info(axis, num_of_axis, button, num_of_buttons);
        }
    }

    close(joy_fd);
    return 0;
}
