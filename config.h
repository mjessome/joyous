#define JOY_DEV         "/dev/input/js0"
#define SHELL           "/bin/sh"

/*
 * KEYPRESS:     Used to send a press/release of a keyboard key. This should be
 *                 coupled with a key argument.
 * RELEASE(fn):  Call function on release of button.
 * PRESS(fn):    Call function on press of button.
 *
 * Note: Multiple actions can be specified for any button.
 */
#define KEYPRESS        {send_key_release, send_key_press}
#define RELEASE(fn)     {fn, NULL}
#define PRESS(fn)       {NULL, fn}
static Button buttons[] = {
    /* button       key up,down             argument */
    { 1,            PRESS(print),           { .s = "Button 1 pressed!" } },
    { 2,            RELEASE(quit),          { .i = 1 } },
    { 3,            RELEASE(exec_cmd),      { .s = "xterm" } },
    { 4,            KEYPRESS,               { .k = { XK_P, XK_Alt_L } } },
    { 5,            {print, print},         { .s = "Button 5!" } },
    { 6,            KEYPRESS,               { .k = { XK_A, 0 } } },
    { 7,            KEYPRESS,               { .k = { XK_Shift_L, 0 } } },
    { 8,            RELEASE(exec_cmd),      { .s = "mpc play" } },
    { 9,            RELEASE(exec_cmd),      { .s = "mpc stop" } },
    { 10,           PRESS(send_string),     { .s = "ExIt\n" } },
};

