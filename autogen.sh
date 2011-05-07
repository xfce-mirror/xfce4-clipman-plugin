#!/bin/sh

(type xdt-autogen) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autogen.sh: You don't seem to have the Xfce development tools installed on
            your system, which are required to build this software.
            Please install the xfce4-dev-tools package first, available from
            http://xfce.org/~benny/projects/xfce4-dev-tools/.
EOF
  exit 1
}

XDT_AUTOGEN_REQUIRED_VERSION="4.7.3" \
exec xdt-autogen $@
