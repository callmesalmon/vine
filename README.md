VINE Editor
===========

VINE (Very INtuitive Editor) is a fast and intuitive terminal-based text editor based on "kilo",
(https://github.com/antirez/kilo), a very small text editor. This version has a *lot* of improvements.
For example more syntax highlighting, more intuitive shortcuts and even configuration files. It also
runs on *just* the C standard library!

**DISCLAIMER**: This is not a UTF-8 text editor, and I have no idea how to implement it. Sue me.

[![VINE EDITOR](https://github.com/callmesalmon/vine/raw/main/vineimg.png)](https://github.com/callmesalmon/vine)  
VINE editing its source code with the "sonokai" colorscheme

Requirements
------------
* <https://gcc.gnu.org/install>
* <https://www.gnu.org/software/make>
* <https://git-scm.com/downloads>

Install
-------
To install, firstly clone the repo:
```sh
git clone https://github.com/callmesalmon/vine ~/vine
```

After that, you'd want to use ``make`` to initialize an executable:
```sh
make # or ``sudo make install`` for installation to /usr/local/bin
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
C-x C-s - Save
C-x C-c - Quit
C-x C-f - Open new file
C-s     - Find
C-k     - Delete next char
C-d     - Delete current line
M-g j   - Start of line
M-g k   - End of line
M-g g   - Goto line
C-c C-s - Run shell command
C-c C-c - Run vine command
```
And before you ask: Yes I stole most of these from EMACS. Like they all say: Every non-modal
terminal editor is destined to become EMACS lite.

Configuration
-------------
The VINE configuration operates using a config file, ``~/.vinerc``.
Comments may be added, starting with ``"`` like in VimScript.

The following is a list of configuration options:
```
tab_size <int>              - Tab size
quit_times <int>            - Amount of times to press <C-Q> until it actully quits
show_empty_lines <bool>     - Whether or not to show tildes on empty lines
expand_tab <bool>           - Whether or not to expand tab to [tab_size] number of spaces.
colorscheme <str>           - Set colorscheme
autopair <bool>             - Match quotes and braces.
```

Sample:
```vim
" Tab formatting
tab_size = 2
expand_tab = true

quit_times = 2

show_empty_lines = false

colorscheme = "kilo"

autopair = true
```
