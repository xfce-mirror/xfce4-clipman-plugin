[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/blob/master/COPYING)

# xfce4-clipman-plugin

Xfce4-clipman-plugin is a plugin for the Xfce Panel and as a standalone application (it is a
bundle). It keeps the clipboard contents around (even after an application
quits). It is able to handle text and images, and has a feature to execute
actions on specific text selection by matching them against regexes.

----

### Homepage

[Xfce4-clipman-plugin documentation](https://docs.xfce.org/panel-plugins/xfce4-clipman-plugin)

### Changelog

See [NEWS](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Required Packages

  * [GTK](https://www.gtk.org)
  * [libxfce4ui](https://gitlab.xfce.org/xfce/libxfce4ui)
  * [libxfce4util](https://gitlab.xfce.org/xfce/libxfce4util)
  * [xfconf](https://gitlab.xfce.org/xfce/xfconf)
  * [xfce4-panel](https://gitlab.xfce.org/xfce/xfce4-panel)

For concrete information on the minimum required versions, check [[https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/blob/master/configure.ac.in|configure.ac.in]]

### Source Code Repository

[Xfce4-clipman-plugin source code](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin)

### Download a Release Tarball

[Xfce4-clipman-plugin archive](https://archive.xfce.org/src/panel-plugins/xfce4-clipman-plugin)
    or
[Xfce4-clipman-plugin tags](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/tags)

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

