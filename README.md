VINE Editor
===========

VINE (Very INtuitive Editor) is a fast and intuitive terminal-based text editor based on "kilo",
(https://github.com/antirez/kilo), a very small text editor. This version has a *lot* of improvements.
For example more syntax highlighting, more intuitive shortcut and even configuration files (this might
just be my opinion, but a much better colourscheme as well). It also runs on *just* the C standard library!

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
```
Ctrl+S - Save
Ctrl+Q - Quit
Ctrl+F - Find
Ctrl+X - Delete next char
Ctrl+D - Delete current line
Ctrl+J - Start of line
Ctrl+K - End of line
Ctrl+O - Open new file
Ctrl+G - Goto line
Ctrl+B - Run bash command
```

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
match_quote_brace <bool>    - Match quotes and braces.
```

Sample:
```vim
" Tab formatting
tab_size = 2
expand_tab = true

quit_times = 2

show_empty_lines = false

colorscheme = "kilo"

match_quote_brace = true
```
