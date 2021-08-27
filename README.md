[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/blob/master/COPYING)

# xfce4-clipman-plugin fork

Protoype disposable code, don't expect a fully running code.

![image.png](./image.png)

Example of cli session

![image-1.png](./image-1.png)

## This version implements the concept of: Secure Item

This version of clipman is a PoC (Proof of Concept) to experiment how we could handle `secure_item`. Item that can be deleted or obfuscated in visual
GUI or via cli.

This code come from an idea discussion on the following [xfce issue #25](https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/issues/25)

## DBus method API

I order to modify clipboard history, in memory, an API (IPC Interprocess Communication) has been implemented.

### `list_history`

Retrieve all items in clipman history (Secure Item are hidden).

Actually return a string. (will be a more complex DBus format)
Format mutiple row separated be newline `\n`:

```
ID TEXT
```

### `get_item_by_id`

Get one item, by its ID in clipman history. ID will be obtained by `list_history` or result of `add_item`. You can read the Secure Item value via a`get_item_by_id` call.

Argument:
 * uint32 id of an item in the history

Returns: string

### `add_item`

Add text item to clipman history

Argument:
 * secure: boolean
 * value: string

Returns: unit16 the new ID of the instered item.

### `delete_item_by_id`

Remove an item from the clipman history if the ID exists.

Returns: boolean

### `clear_history`

Remove all data in clipman history.

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
./clipman_cli.sh list
```

### read a single item by id from the clipman history through DBus call

```
./clipman_cli.sh get 123
```

### delete a single item by id from the clipman history through DBus call

```
./clipman_cli.sh del 123
```

### add a single item into the clipman history through DBus call

```
./clipman_cli.sh add "my content here"
```

`secure_item`

```
./clipman_cli.sh add -s "my secure_item content here"
```

## Roadmap in clipman modification

This is just some suggestion, I don't know the project enough for now to be accurate:

* ~~add remote call behavior IPC to clipman~~ done with dbus in this PoC
* ~~ensure all the entries have permanent auto incremented ids (even when sorted or deleted)~~ draft done in the PoC
* ~~retrieve an item in the clipman history by id~~ done with dbus in this PoC
* ~~find a way how to delete a given entry in clipman~~ done with dbus in this PoC
* ~~add a DBus method to add item in history through DBus~~ done with dbus in this PoC
* ~~find way to store a new `secure_item` in clipman (type: secure + text value)~~ done with dbus in this PoC
* ~~gui change: obfuscate  `secure_item` in popup history~~ done with dbus in this PoC
* ~~add a DBus method to clear all history~~  done with dbus in this PoC
* add a DBus parameter to clear all `secure_item` only.
* delete an Item from the GUI menu
* toggle an Item in the GUI menu as Secure


## How to build

Dont forget to install the lovely colored emoji font!  😀

```
sudo apt install xfce4-dev-tools libgtk-3-dev libxfce4ui-2-dev libxfce4panel-2.0-dev fonts-emojione
./autogen.sh --enable-debug
make
# prefix in /usr/local by default, so it may require sudo to work
make install
```


## Xfce dev question

* Changing GSList by GList (double linked list) for simpler removal of item?
* What signal to emmit when item are removed?
* How to map delete key, so we can delete an Item from the menu
* What is the dbus: session bus: org.xfce.clipman?
