#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <unistd.h>

int
main (void)
{
  Display *display = XOpenDisplay (NULL);
  int dummyi;
  if (display == NULL)
    return 1;
  printf ("XTest: %d\n", XQueryExtension (display, "XTEST", &dummyi, &dummyi, &dummyi));

  int ret = -1;
  KeySym key_sym;
  KeyCode key_code;
  key_sym = XStringToKeysym ("1");
  key_code = XKeysymToKeycode (display, key_sym);
  XTestFakeKeyEvent (display, key_code, True, CurrentTime);
  ret = XTestFakeKeyEvent (display, key_code, False, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XStringToKeysym ("2");
  key_code = XKeysymToKeycode (display, key_sym);
  XTestFakeKeyEvent (display, key_code, True, CurrentTime);
  ret = XTestFakeKeyEvent (display, key_code, False, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XStringToKeysym ("3");
  key_code = XKeysymToKeycode (display, key_sym);
  XTestFakeKeyEvent (display, key_code, True, CurrentTime);
  ret = XTestFakeKeyEvent (display, key_code, False, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XK_Return;
  key_code = XKeysymToKeycode (display, key_sym);
  XTestFakeKeyEvent (display, key_code, True, CurrentTime);
  ret = XTestFakeKeyEvent (display, key_code, False, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XK_Control_L;
  key_code = XKeysymToKeycode (display, key_sym);
  ret = XTestFakeKeyEvent (display, key_code, True, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XK_v;
  key_code = XKeysymToKeycode (display, key_sym);
  XTestFakeKeyEvent (display, key_code, True, CurrentTime);
  ret = XTestFakeKeyEvent (display, key_code, False, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XK_Control_L;
  key_code = XKeysymToKeycode (display, key_sym);
  ret = XTestFakeKeyEvent (display, key_code, False, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XK_Shift_L;
  key_code = XKeysymToKeycode (display, key_sym);
  ret = XTestFakeKeyEvent (display, key_code, True, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XK_Insert;
  key_code = XKeysymToKeycode (display, key_sym);
  ret = XTestFakeKeyEvent (display, key_code, True, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XK_Insert;
  key_code = XKeysymToKeycode (display, key_sym);
  ret = XTestFakeKeyEvent (display, key_code, False, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  key_sym = XK_Shift_L;
  key_code = XKeysymToKeycode (display, key_sym);
  ret = XTestFakeKeyEvent (display, key_code, False, CurrentTime);
  printf ("XTestFakeKeyEvent (%s): %d\n", XKeysymToString (key_sym), ret);

  printf ("Closing display in 5s...\n");
  usleep (5000000);
  XCloseDisplay (display);

  return 0;
}
