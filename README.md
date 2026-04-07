VINE Editor
===========

VINE (Very INtuitive Editor) is a fast and intuitive terminal-based text editor based on "kilo",
(https://github.com/antirez/kilo), a very small text editor. This version has a *lot* of improvements.
For example more syntax highlighting, more intuitive shortcuts and even configuration files. While I
*try* to keep the main spirit of Kilo intact, VINE *does* differ in many very fundamental ways, for
example it does change the keyboard shortcuts a lot.

**DISCLAIMER**: This is not a UTF-8 text editor (in fact, VINE, will not even let you type non-ASCII characters),
and I have no idea how to implement it. Sue me.

[![VINE EDITOR](https://github.com/callmesalmon/vine/raw/main/vineimg.png)](https://github.com/callmesalmon/vine)  
VINE editing its source code with the default colorscheme (image may be outdated)

Requirements
------------
* <https://gcc.gnu.org/install>
* <https://www.gnu.org/software/make>
* <https://git-scm.com/downloads>

Install
-------
```sh
git clone https://github.com/callmesalmon/vine ~/vine
cd ~/vine
make install # or just "make" if you just want to test out the editor!
```

Uninstall
---------
```sh
make clean
```

Usage
-----
```sh
vine <filename>
```

Keybinds
--------
Since this is key notation, ``C-x`` means ``Ctrl + x`` and ``M-x`` means ``Meta + x``/``Alt + x``
```
C-x C-s Save
C-x C-c Quit
C-x C-f Open new file
C-c C-s Run shell command
C-c C-c Run vine command
C-z     Suspend VINE
C-s     Find
C-d     Delete next char
C-k     Delete current line
C-h     Open help screen
M-f     Move forward one word
M-b     Move backward one word
C-a     Start of line
C-e     End of line
M-a     Start of file
M-e     End of file
M-g     Goto line
```
And before you ask: Yes I stole most of these from EMACS. Like they all say: Every non-modal
terminal editor is destined to become EMACS lite.

Configuration
-------------
The VINE configuration operates using a config file, ``~/.vinerc``.
Comments may be added, starting with ``"`` like in VimScript.

The following is a list of configuration options:
```
tab-size <int>              Tab size
show-empty-lines <bool>     Whether or not to show tildes on empty lines
expand-tab <bool>           Whether or not to expand tab to [tab_size] number of spaces.
colorscheme <str>           Set colorscheme
autopair <bool>             Match quotes and braces.
colr-{x} <color>            Set color of highlight-fields.
```

Sample:
```vim
" Tab formatting
tab-size = 2
expand-tab = true

show-empty-lines = false

colorscheme = "kilo"

autopair = true
```
