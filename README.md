[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/blob/master/COPYING)

# xfce4-clipman-plugin

Xfce4-clipman-plugin is a plugin for the Xfce Panel and as a standalone application (it is a
bundle). It keeps the clipboard contents around (even after an application
quits). It is able to handle text and images, and has a feature to execute
actions on specific text selection by matching them against regexes.

----

### Homepage

[Xfce4-clipman-plugin documentation](https://docs.xfce.org/panel-plugins/xfce4-clipman-plugin)

## This version try to implement: Secure Item

This version of clipman is a PoC to experiment how to handle `secure_item` that can be deleted or obfuscated in visual
GUI or via cli.

This code come from an idea discussion on the follow [xfce issue #25](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/issues/25)

## DBus method API

### `list_history`

Retrieve all item in clipman history.

Actually return a string. (will be a more complex DBus format)
Format mutiple row separated be newline `\n`:

```
ID TEXT
```

### `get_item_by_id`

Get one item, by its id in history. Id will be obtained by `list_history` or result of `add_item`

Argument:
 * uint16 id of an item in the history

### `add_item`

*NOT IMPLEMENTED*

Add text item to clipman history

Argument:
 * secure: boolean
 * value: string

## Secure item disussion

We are interested in the feature that would handle password copied to the clipman clipboard history in a secure maner.
Those secure item should not be exposed, or should be deleted automatically after a short period (30s for example).

### My context

For now I'm using `pass` too + my own variant of dmenu shell script (was [https://git.zx2c4.com/password-store/tree/contrib/dmenu](https://git.zx2c4.com/password-store/tree/contrib/dmenu))

But now, I use [rofi](https://github.com/davatorium/rofi) instead of `dmenu`, and I put multiple entries in the
clipboard, which is possible with a clipboard manager. But this disable the autoremoval delay in the clipboard.

> rofi is a dmenu drop in replacement tool also available in package distrib.

### Secure clipboard manager

Now, I'm also facing ISO 27001 certification, and I would like a more secure clipboard. And I would love to continue using clipman, too.

Here is my propositions to handle secure clipboard (not so secure, but with some extended support to make it more safe).


* add a new `secure_item` type in clipman (in plus of image and text item storage)
* via a new command line interface we manage the clipboard history:
  * delete an entry by ID: `xfce4-clipman-cli delete 123` (delete entry numbered 123 from history)
  * delete an entry by content: `xfce4-clipman-cli delete -c "$password_value"`
  * list entries: `xfce4-clipman-cli list` (output text entry with id as prefix)
  * set an item of the clipman history as `secure`: `xfce4-clipman-cli secure 123` (make item number 123 as secure)
  * insert a secure item directly: `xfce4-clipman-cli add --secure "$password"` (output the new id inserted item)


History deletion could be managed outside clipman by secure storage manager,
like `pass` or wrapper helper script. So clipman don't have to handle
timestamping and to delete item itself. It's less invasive in the climan code
and let user to configure their own script.

Actually the last sentence is fasle. At the time I started reading climap code, there was a `xfce4-clipman-history` but
this code cannot communicate with the clipman data in memory. That was fixed by introducing DBus API.

### `secure_item`

`secure_item` is a text item inserted in clipman history, but hidden (or
masked) in the history display on gui. May be on list too `xfce4-clipman-cli list`
or could be shown with an extra switch
`xfce4-clipman-cli list --show-secure-item`.

## Usage examples

DBus shell wrapper. Prototype disposable script for testing the PoC


### read the clipman history through DBus call

```
./read_dbus_history.sh
```

### read a single item by id from the clipman history through DBus call

```
./clipman_history_get_value.sh 123
```


### fake usage

With a dummy shell script simulating what the real code could be:

```
./xfce4-clipman-cli.sh -l
58 xfce4-clipman-cli
59 delete an entry
60 ~/.cache/xfce4/clipman
61 1.6.1git-c71f6f7
62 C: #no just kiding
63 https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/issues/25
64 IMAGE
65 LONG_TEXT(598) via the command line helper to manage cl ...
66 SECURE ***********
```

```
./xfce4-clipman-cli.sh -l --show-secure-item
58 xfce4-clipman-cli
59 delete an entry
60 ~/.cache/xfce4/clipman
61 1.6.1git-c71f6f7
62 C: #no just kiding
63 https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/issues/25
64 IMAGE
65 LONG_TEXT(598) via the command line helper to manage cl ...
66 weapon5Riot2Cold
```

## Roadmap in clipman modification

This is just some suggestion, I don't know the project enough for now to be accurate:

* ~~add remote call behavior to clipman~~ done with dbus on this PoC
* ~~ensure all the entries have permanent auto incremented ids (even when sorted or deleted)~~ draft done in the PoC
* ~~retrieve an item in the clipman history by id~~ done with dbus on this PoC
* find a way how to delete a given entry in clipman
* find way to store a new `secure_item` in clipman (type: secure + text value)
* gui change: obfuscate  `secure_item` in popup history


## How to build

```
sudo apt install xfce4-dev-tools libgtk-3-dev libxfce4ui-2-dev libxfce4panel-2.0-dev
./autogen.sh --enable-debug
make
# prefix in /usr/local by default, so it may require sudo to work
make install
```
