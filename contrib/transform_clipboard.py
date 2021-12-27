#!/usr/bin/env python

import sys


html_input = sys.stdin.read().strip()

html_open_table = """
<table width="700" cellspacing="2" cellpadding="2" border="1">
  <tbody>
    <tr>
      <td valign="top" bgcolor="#000000">
""".strip()

html_close_table = """
      </td>
    </tr>
  </tbody>
</table>
<br>
""".strip()

default_html_color = "#b7b7b7"

# replace <pre>
if html_input.startswith("<pre>"):
    html_input = html_input.replace("<pre>", ('<pre><font color="%s">' % default_html_color))
    html_input = html_input.replace("</pre>", '</font></pre>')
else:
    html_input = '<pre><font color="%s">%s</font></pre>' % (default_html_color, html_input)

print("%s%s%s" % (html_open_table, html_input, html_close_table))
