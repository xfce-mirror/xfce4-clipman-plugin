[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/xfce/xfce4-clipman-plugin/COPYING)

# xfce4-clipman-plugin

Clipman is a plugin for the Xfce Panel and as a standalone application (it is a
bundle). It keeps the clipboard contents around (even after an application
quits). It is able to handle text and images, and has a feature to execute
actions on specific text selection by matching them against regexes.

----

### Homepage

[xfce4-clipman-plugin documentation](https://docs.xfce.org/panel-plugins/xfce4-clipman-plugin)

### Changelog

See [NEWS](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Required Packages

* glib-2.0 >=2.32
* gtk+-3.0 >=3.14
* libxfce4ui-2 >=4.12
* libxfce4panel-2.0 >=4.12
* libxfconf-0 >=4.10
* xproto >=7.0
* xtst >=1.0

## Optional Packages

* unique-3.0 >=3.0
* libqrencode3 >=3.3

### Source Code Repository

[xfce4-clipman-plugin source code](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin)

### Download A Release Tarball

[xfce4-clipman-plugin archive](https://archive.xfce.org/src/panel-plugins/xfce4-clipman-plugin)
    or
[xfce4-clipman-plugin tags](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/tags)

### Installation

From source code repository: 

    % cd xfce4-clipman-plugin
    % ./autogen.sh
    % make
    % make install

From release tarball:

    % tar xf xfce4-clipman-plugin-<version>.tar.bz2
    % cd xfce4-clipman-plugin-<version>
    % ./configure
    % make
    % make install

### Reporting Bugs

Visit the [reporting bugs](https://docs.xfce.org/panel-plugins/xfce4-clipman-plugin/bugs) page to view currently open bug reports and instructions on reporting new bugs or submitting bugfixes.

