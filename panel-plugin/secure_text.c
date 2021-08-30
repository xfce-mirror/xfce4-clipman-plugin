#include "secure_text.h"

gchar * clipman_secure_text_decode(const gchar *secure_text)
{
  guchar *decoded_content = NULL;

  // returns FALSE with CLIPMAN_SECURE_TEXT_MAX_LEN (1024) for gssize??
  if (g_utf8_validate (secure_text, -1, NULL))
    {
      gsize out_len;
      gunichar marker = g_utf8_get_char(secure_text);
      if (marker == CLIPMAN_SECURE_TEXT_MARKER)
      {
        decoded_content = g_base64_decode (g_utf8_next_char(secure_text), &out_len);
      }
    }
  else
  {
    g_print("g_utf8_validate, returned false: %s", secure_text);
  }

  return (gchar*) decoded_content;
}

gchar * clipman_secure_text_encode(const gchar *clear_text)
{
  gchar *encoded_content = NULL;

  if (g_utf8_validate (clear_text, -1, NULL))
    {
      gsize out_len;
      guchar *tmp;

      // don't use utf8 method we want full size of the string
      tmp = g_base64_encode ((guchar*) clear_text, g_strv_length(clear_text));
      encoded_content = g_strdup_printf ("%c%s", CLIPMAN_SECURE_TEXT_MARKER, tmp);
      g_free(tmp);
    }
  else
  {
    g_print("g_utf8_validate, returned false: %s", clear_text);
  }

  return encoded_content;
}
