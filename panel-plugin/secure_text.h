#ifndef __CLIPMAN_SECURE_TEXT_H
#define __CLIPMAN_SECURE_TEXT_H

#include <gtk/gtk.h>

// utf-8 symbols  Wrong way sign:  0x26d4 ⛔
#define CLIPMAN_SECURE_TEXT_MARKER  0x26d4
#define CLIPMAN_SECURE_TEXT_MAX_LEN 1024

gchar * clipman_secure_text_decode(const gchar *secure_text);
gchar * clipman_secure_text_encode(const gchar *clear_text);

#endif /* __CLIPMAN_SECURE_TEXT_H */
