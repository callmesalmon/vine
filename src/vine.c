/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (c) 2024 Salmon                                                 *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * VINE EDITOR                                                               *
 *                                                                           *
 * Vine is a fork of the modified version of "Kilo" by Snaptoken,            *
 * which is licensed under the BSD-Clause-2 License, while all               *
 * of the modified pieces of code are released Apache License 2.0.           *
 * Please abide to the fitting licenses under the fitting                    *
 * circumstances.                                                            *
 *                                                                           *
 * Thank you Salvatore Sanfilippo (antirez on github.com) for                *
 * making the original "Kilo" editor & thank you Paige Ruten                 *
 * (paigeruten on github.com, but the project I forked is on her             *
 * "tutorial account"; snaptoken) for making the slightly modified           *
 * version. Speaking about Snaptoken, she has a really good tutorial         *
 * on Kilo, which I followed to make this editor, check it out on            *
 * <https://viewsourcecode.org/snaptoken/kilo>,                              *
 * it'll be worth your time.                                                 *
 *                                                                           *
 * This is the Vine source code, feel free to contribute on                  *
 * <https://github.com/callmesalmon/vine>. Or you can just enjoy             *
 * the editor, I won't judge! And with that, enjoy!                          *
 *                                                                           *
 * Vine was made by Salmon and released under the                            *
 * Apache-2.0-license (but the original kilo is licensed                     *
 * under the BSD-clause-2-license, so you also have to                       *
 * follow that license for the original, unmodified                          *
 * parts of the source code). Enjoy!                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define VINE_VERSION "NET/1"
#define VINE_LINE_NUMBER_PADDING 4
#define VINE_QUIT_TIMES 3

/* This mimics the Ctrl key by switching the
 * 3 upper bits of the key pressed to 0 */
#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
    BACKSPACE  = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

enum editorHighlight {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

/* ==================== Data ==================== */

struct editorSyntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singleline_comment_start;
    char *multiline_comment_start;
    char *multiline_comment_end;
    int  flags;
};

struct editorTheme {
    int hl_com;
    int hl_kw1;
    int hl_kw2;
    int hl_str;
    int hl_num;
    int hl_find;
    int hl_nil;
};

typedef struct erow {
    int           idx;
    int           size;
    int           rsize;
    char          *chars;
    char          *render;
    unsigned char *hl;
    int           hl_open_comment;
} erow;

struct editorConfig {
    int    cx, cy;
    int    rx;
    int    rowoff;
    int    coloff;
    int    screenrows;
    int    screencols;
    int    numrows;
    erow   *row;
    int    dirty;
    char   *filename;
    char   statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;
    struct termios orig_termios;
    int    tab_stop;
    int    quit_times;
};

struct editorConfig E;

/* ==================== Themes ======================
 *
 * In order add a new theme, define a new (const) struct out of the struct
 * "editorTheme", then add all of the colour values into it in the form of a
 * FG-ANSI colour. Then, just simply change the values of the "T" struct to
 * your themes value (see "sonokai" theme example below).
 */

const struct editorTheme sonokai = {
    90, 31, 32, 92,
    35, 34, 37,
};

struct editorTheme T = {
    sonokai.hl_com, sonokai.hl_kw1, sonokai.hl_kw2, sonokai.hl_str,
    sonokai.hl_num, sonokai.hl_find, sonokai.hl_nil
};

/* ==================== Syntax Highlighting ====================
 *
 * In order to add a new syntax, define two arrays with a list of file name
 * matches and keywords. The file name matches are used in order to match
 * a given syntax with a given file name: if a match pattern starts with a
 * dot, it is matched as the last past of the filename, for example ".c".
 * Otherwise the pattern is just searched inside the filename, like "Makefile".
 *
 * The list of keywords to highlight is just a list of words, however if
 * there's a trailing '|' character added at the end, they are highlighted in
 * a different color, so that you can have two different sets of keywords.
 *
 * Finally add a stanza in the HLDB global variable with two arrays
 * of strings, and a set of flags in order to enable highlighting of
 * comments and numbers.
 *
 * The characters for single and multi line comments must be exactly two
 * and must be provided as well (see the C language for example).
 *
 * The default syntax highlighted languages are: C, Python, Go, Rust
 */

char *C_HL_extensions[] = { ".c", ".h", ".cpp", ".hpp",
                            ".cc", ".hh", ".cxx", ".hxx", NULL };
char *C_HL_keywords[] = {
    "auto", "break", "case", "const", "continue", "default", "do", "else", "enum",  "extern",
    "for", "goto", "if", "register", "return", "sizeof", "static",  "struct", "switch", 
    "typedef", "union", "volatile", "while", "__asm__", "NULL", "alignas", "alignof",
    "and", "and_eq", "asm", "bitand", "bitor", "class",  "compl", "constexpr",
    "const_cast", "deltype", "delete", "dynamic_cast",  "explicit", "export", "false",
    "friend", "inline", "mutable", "using", "namespace",  "new", "noexcept", "not",
    "not_eq", "nullptr", "operator", "or", "or_eq",  "private", "protected", "public",
    "reinterpret_cast", "static_assert", "static_cast", "template", "this",
    "thread_local", "throw", "true", "try", "typeid", "typename", "virtual",
    "xor", "xor_eq", "#define", "#include", "#if", "ifdef", "#ifndef",
    "#endif", "#error", "#warning", "#pragma",

    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", "short|", "auto|", "bool|", NULL
};

char *GO_HL_extensions[] = { ".go", NULL };
char *GO_HL_keywords[] = {
    "if", "else", "switch", "case", "func", "then", "for", "var", "type", "interface", "const", "range",
    "return", "struct", "default", "iota", "nil", "package", "import", "map", "break", "continue",

    "int", "int8", "int16", "int32", "int64", "uint", "uint8", "uint16", "uint32", "uint64",
    "float32", "float64", "byte", "rune", "bool", "string", "complex64", "complex128",
    "any", "error", "comparable", NULL
};

char *PY_HL_extensions[] = { ".py", "pyi", ".xpy", "pyx",
                             ".pyw", ".ipynb", NULL };
char *PY_HL_keywords[] = {
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif",
    "else", "except", "exec", "finally", "for", "from", "global", "if", "import",
    "in", "is", "lambda", "not", "or", "pass", "print", "raise", "return", "try",
    "while", "with", "yield", "async", "await", "nonlocal", "range", "xrange",
    "reduce", "map", "filter", "all", "any", "sum", "dir", "abs", "breakpoint",
    "compile", "delattr", "divmod", "format", "eval", "getattr", "hasattr",
    "hash", "help", "id", "input", "isinstance", "issubclass", "len", "locals",
    "max", "min", "next", "open", "pow", "repr", "reversed", "round", "setattr",
    "slice", "sorted", "super", "vars", "zip", "__import__", "reload", "raw_input",
    "execfile", "file", "cmp", "basestring",

    "buffer|", "bytearray|", "bytes|", "complex|", "float|", "frozenset|", "int|",
    "list|", "long|", "None|", "set|", "str|", "chr|", "tuple|", "bool|", "False|",
    "True|", "type|", "unicode|", "dict|", "ascii|", "bin|", "callable|",
    "classmethod|", "enumerate|", "hex|", "oct|", "ord|", "iter|", "memoryview|",
    "object|", "property|", "staticmethod|", "unichr|", NULL
};

char *RUST_HL_extensions[] = { ".rs", NULL };
char *RUST_HL_keywords[] = {
    "as", "async", "await", "const", "crate", "dyn", "enum", "extern", "fn", "impl", "let",
    "mod", "move", "mut", "pub", "ref", "Self", "static", "struct", "super", "trait", "type",
    "union", "unsafe", "use", "where", "break", "continue", "else", "for", "if", "in", "loop",
    "match", "return", "while",

    "i8|", "i16|", "i32|", "i64|", "i128|", "isize|", "u8|", "u16|", "u32|", "u64|", "u128|", "usize|",
    "f32|", "f64|", "bool|", "char|", "Box|", "Option|", "Some|", "None|", "Result|", "Ok|", "Err|",
    "String|", "Vec|", "let|", "const|", "mod|", "struct|", "enum|", "trait|", "union|", "self|",
    "true|", "false|", NULL
};

/* HLDB stands for HighLighting DataBase, and contains
 * the settings and initialization for the syntax highlighting */
struct editorSyntax HLDB[] = {
    {
        "C/C++",
        C_HL_extensions,
        C_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS,
    },
    {
        "Golang",
        GO_HL_extensions,
        GO_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS,
    },
    {
        "Python",
        PY_HL_extensions,
        PY_HL_keywords,
        "#", "\"\"\"", "\"\"\"",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS,
    },
    {
        "Rust",
        RUST_HL_extensions,
        RUST_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS,
    },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(*HLDB))

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));

/* ==================== Low level terminal handling ==================== */

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag        &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag        &= ~(OPOST);
    raw.c_cflag        |= (CS8);
    raw.c_lflag        &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN]     = 0;
    raw.c_cc[VTIME]    = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/* ==================== Syntax Highlighting ==================== */

int is_separator(int c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*^=@#~&%$`´<>[]{}!\\:|;?", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if (E.syntax == NULL) return;

    char **keywords = E.syntax->keywords;

    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;

    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;

    int prev_sep   = 1;
    int in_string  = 0;
    int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);

    int i = 0;
    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

        if (scs_len && !in_string && !in_comment) {
            if (!strncmp(&row->render[i], scs, scs_len)) {
                memset(&row->hl[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }

        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                row->hl[i] = HL_MLCOMMENT;
                if (!strncmp(&row->render[i], mce, mce_len)) {
                    memset(&row->hl[i], HL_MLCOMMENT, mce_len);
                    i          += mce_len;
                    in_comment = 0;
                    prev_sep   = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
                memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
                i          += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                row->hl[i] = HL_STRING;
                if (c == '\\' && i + 1 < row->rsize) {
                    row->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string) in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            } else {
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
                    ((c == '.' || c == 'x' || c == 'a' || c == 'b' ||
                    c == 'c' || c == 'd' || c == 'e' || c == 'f') &&
                    prev_hl == HL_NUMBER)) {
                row->hl[i] = HL_NUMBER;
                i++;
                prev_sep = 0;
                continue;
            }
        }

        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2) klen--;

                if (!strncmp(&row->render[i], keywords[j], klen) &&
                        is_separator(row->render[i + klen])) {
                    memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue;
            }
        }

        prev_sep = is_separator(c);
        i++;
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < E.numrows)
        editorUpdateSyntax(&E.row[row->idx + 1]);
}

int editorSyntaxToColour(int hl) {
    switch (hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT: return T.hl_com;
    case HL_KEYWORD1:  return T.hl_kw1;
    case HL_KEYWORD2:  return T.hl_kw2;
    case HL_STRING:    return T.hl_str;
    case HL_NUMBER:    return T.hl_num;
    case HL_MATCH:     return T.hl_find;
    default:           return T.hl_nil;
    }
}

void editorSelectSyntaxHighlight() {
    E.syntax = NULL;
    if (E.filename == NULL) return;

    char *ext = strrchr(E.filename, '.');

    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct editorSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s->filematch[i]) {
            int is_ext = (s->filematch[i][0] == '.');
            if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
                    (!is_ext && strstr(E.filename, s->filematch[i]))) {
                E.syntax = s;

                int filerow;
                for (filerow = 0; filerow < E.numrows; filerow++) {
                    editorUpdateSyntax(&E.row[filerow]);
                }

                return;
            }
            i++;
        }
    }
}

/* ==================== Row Operations ==================== */

int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (E.tab_stop - 1) - (rx % E.tab_stop);
        rx++;
    }
    return rx;
}

int editorRowRxToCx(erow *row, int rx) {
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < row->size; cx++) {
        if (row->chars[cx] == '\t')
            cur_rx += (E.tab_stop - 1) - (cur_rx % E.tab_stop);
        cur_rx++;

        if (cur_rx > rx) return cx;
    }
    return cx;
}

void editorUpdateRow(erow *row) {
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;

    free(row->render);
    row->render = malloc(row->size + tabs*(E.tab_stop - 1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % E.tab_stop != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;

    editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
    if (at < 0 || at > E.numrows) return;

    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
    for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

    E.row[at].idx   = at;
    E.row[at].size  = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize           = 0;
    E.row[at].render          = NULL;
    E.row[at].hl              = NULL;
    E.row[at].hl_open_comment = 0;
    editorUpdateRow(&E.row[at]);

    E.numrows++;
    E.dirty++;
}

void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

void editorDelRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
    E.numrows--;
    E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

/* ==================== Editor Operations ==================== */

void editorInsertChar(int c) {
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorInsertNewline() {
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    } else {
        erow *row = &E.row[E.cy];
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row                   = &E.row[E.cy];
        row->size             = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void editorDelChar() {
    if (E.cy == E.numrows ||
       (E.cx == 0 && E.cy == 0)) return;

    erow *row = &E.row[E.cy];
    if (E.cx > 0) {
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    } else {
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}

/* ==================== I/O ==================== */

char *editorRowsToString(int *buflen) {
    int totlen = 0;
    int j;
    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size + 1;
    *buflen = totlen;

    char *buf = malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void editorOpen(char *filename) {
    free(E.filename);
    E.filename = strdup(filename);

    editorSelectSyntaxHighlight();

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("[ERROR]: The requested file could not be opened.");
        return;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
}

void editorSave() {
    if (E.filename == NULL) {
        E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
        if (E.filename == NULL) {
            editorSetStatusMessage("Save aborted");
            return;
        }
        editorSelectSyntaxHighlight();
    }

    int len;
    char *buf = editorRowsToString(&len);

    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                E.dirty = 0;
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editorSetStatusMessage("[ERROR]: Can't save! I/O error: %s", strerror(errno));
}

void editorFindCallback(char *query, int key) {
    static int last_match = -1;
    static int direction  = 1;

    static int saved_hl_line;
    static char *saved_hl = NULL;

    if (saved_hl) {
        memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction  = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    } else {
        last_match = -1;
        direction  = 1;
    }

    if (last_match == -1) direction = 1;
    int current = last_match;
    int i;
    for (i = 0; i < E.numrows; i++) {
        current += direction;
        if (current == -1) current = E.numrows - 1;
        else if (current == E.numrows) current = 0;

        erow *row = &E.row[current];
        char *match = strstr(row->render, query);
        if (match) {
            last_match = current;
            E.cy     = current;
            E.cx     = editorRowRxToCx(row, match - row->render);
            E.rowoff = E.numrows;

            saved_hl_line = current;
            saved_hl      = malloc(row->rsize);
            memcpy(saved_hl, row->hl, row->rsize);
            memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void editorFind() {
    int saved_cx     = E.cx;
    int saved_cy     = E.cy;
    int saved_coloff = E.coloff;
    int saved_rowoff = E.rowoff;

    char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)",
                               editorFindCallback);

    if (query) {
        free(query);
    } else {
        E.cx     = saved_cx;
        E.cy     = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_rowoff;
    }
}

/* ==================== Terminal Update =================== */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

void editorScroll() {
    E.rx = 0;
    if (E.cy < E.numrows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }

    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.rx < E.coloff) {
        E.coloff = E.rx;
    }
    if (E.rx >= E.coloff + E.screencols) {
        E.coloff = E.rx - E.screencols + 1;
    }
    if (E.rx >= E.coloff + E.screencols - VINE_LINE_NUMBER_PADDING - 1) {
        E.coloff = E.rx - E.screencols + VINE_LINE_NUMBER_PADDING + 2;
    }
}

void editorDrawRows(struct abuf *ab) {
    int y;
    /* TODO: Add more lines with useful
     * info (like help or something). */
    for (y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 3) {
                char welcome[128];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                "Vine editor -- version %s", VINE_VERSION);
                if (welcomelen > E.screencols) welcomelen = E.screencols;
                    int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                  abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1); 
      }
      } else {
      char linenum[32];
      int linenum_len = snprintf(linenum, sizeof(linenum), "%*d ", VINE_LINE_NUMBER_PADDING, filerow + 1);
      if (linenum_len > VINE_LINE_NUMBER_PADDING + 1) {
        linenum_len = VINE_LINE_NUMBER_PADDING + 1;
      }
      abAppend(ab, linenum, linenum_len);

      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0) len = 0;
      if (len > E.screencols - linenum_len) len = E.screencols - linenum_len;

      char *c = &E.row[filerow].render[E.coloff];
      unsigned char *hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      int j;
      for (j = 0; j < len; j++) {
        if (iscntrl(c[j])) {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abAppend(ab, "\x1b[7m", 4);
          abAppend(ab, &sym, 1);
          abAppend(ab, "\x1b[m", 3);
       if (current_color != -1) {
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
            abAppend(ab, buf, clen);
          }
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != -1) {
            abAppend(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          abAppend(ab, &c[j], 1);
        } else {
          int color = editorSyntaxToColour(hl[j]);
          if (color != current_color) {
            current_color = color;
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
            abAppend(ab, buf, clen);
          }
          abAppend(ab, &c[j], 1);
        }
      }
      abAppend(ab, "\x1b[39m", 5);
    }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}


void editorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       E.filename ? E.filename : "[No Name]", E.numrows,
                       E.dirty ? "[+]" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
                        E.syntax ? E.syntax->filetype : "[No FT]", E.cy + 1, E.numrows);
    if (len > E.screencols) len = E.screencols;
    abAppend(ab, status, len);
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() {
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  int linenum_len = snprintf(NULL, 0, "%d", E.numrows);
  if (linenum_len < VINE_LINE_NUMBER_PADDING) linenum_len = VINE_LINE_NUMBER_PADDING;
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
                                            (E.rx - E.coloff) + linenum_len + 2);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}


void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();
        if (c == CTRL_KEY('x') || c == BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        } else if (c == '\x1b') {
            editorSetStatusMessage("");
            if (callback) callback(buf, c);
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editorSetStatusMessage("");
                if (callback) callback(buf, c);
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }

        if (callback) callback(buf, c);
    }
}

void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch (key) {
    case ARROW_LEFT:
        if (E.cx != 0) {
            E.cx--;
        } else if (E.cy > 0) {
            E.cy--;
            E.cx = E.row[E.cy].size;
        }
        break;
    case ARROW_RIGHT:
        if (row && E.cx < row->size) {
            E.cx++;
        } else if (row && E.cx == row->size) {
            E.cy++;
            E.cx = 0;
        }
        break;
    case ARROW_UP:
        if (E.cy != 0) {
            E.cy--;
        }
        break;
    case ARROW_DOWN:
        if (E.cy < E.numrows) {
            E.cy++;
        }
        break;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

void editorProcessKeypress() {
    static int quit_times = VINE_QUIT_TIMES;

    int c = editorReadKey();

    switch (c) {
    case '\r':
        editorInsertNewline();
        break;

    case CTRL_KEY('q'):
        if (E.dirty && quit_times > 0) {
            editorSetStatusMessage("[WARNING]: File has unsaved changes. "
                                   "Press Ctrl-Q %d more times to quit.", quit_times);
            quit_times--;
            return;
        }
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;

    case CTRL_KEY('s'):
        editorSave();
        break;

    case CTRL_KEY('j'):
        E.cx = 0;
        break;

    case CTRL_KEY('k'):
        if (E.cy < E.numrows)
            E.cx = E.row[E.cy].size;
        break;

    case CTRL_KEY('d'):
        editorDelRow(E.cy);
        break;

    case CTRL_KEY('f'):
        editorFind();
        break;

    case BACKSPACE:
    case CTRL_KEY('x'):
        if (c == CTRL_KEY('x')) editorMoveCursor(ARROW_RIGHT);
        editorDelChar();
        break;

    case PAGE_UP:
    case PAGE_DOWN:
    {
        if (c == PAGE_UP) {
            E.cy = E.rowoff;
        } else if (c == PAGE_DOWN) {
            E.cy = E.rowoff + E.screenrows - 1;
            if (E.cy > E.numrows) E.cy = E.numrows;
        }

        int times = E.screenrows;
        while (times--)
            editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    }
    break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        break;

    case CTRL_KEY('l'):
    case '\x1b':
        break;

    default:
        editorInsertChar(c);
        break;
    }

    quit_times = E.quit_times;
}

/* ==================== Init ==================== */

void loadConfig() {
    FILE *file = fopen(strcat(getenv("HOME"), ".vinerc"), "r");
    if (!file) {
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *equals = strchr(line, '=');
        if (equals) {
            *equals     = '\0'; /* Null-terminate at '=' to split key */
            char *key   = line;
            char *value = equals + 1;

            key[strcspn(key, "\r\n")]     = 0;
            value[strcspn(value, "\r\n")] = 0;

            if (strcmp(key, "tab_stop") == 0) {
                E.tab_stop = atoi(value);
            } else if (strcmp(key, "quit_times") == 0) {
                E.quit_times = atoi(value);
            }
        }
    }

    fclose(file);
}

void initEditor() {
    E.cx             = 0;
    E.cy             = 0;
    E.rx             = 0;
    E.rowoff         = 0;
    E.coloff         = 0;
    E.numrows        = 0;
    E.row            = NULL;
    E.dirty          = 0;
    E.filename       = NULL;
    E.statusmsg[0]   = '\0';
    E.statusmsg_time = 0;
    E.syntax         = NULL;
    E.tab_stop       = 4;
    E.quit_times     = 3;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
    E.screenrows -= 2;
}

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();

    if (argc >= 2) {
        editorOpen(argv[1]);
    }

    editorSetStatusMessage(
        "HELP: Ctrl-S = Save | Ctrl-Q = Quit | Ctrl-F = Find");

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
