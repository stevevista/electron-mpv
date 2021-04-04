CONSOLE
=======

The console is a REPL for mpv input commands. It is displayed on the video
window. It also shows log messages. It can be disabled entirely using the
``--load-osd-console=no`` option.

Keybindings
-----------

\`
    Show the console.

ESC
    Hide the console.

ENTER
    Run the typed command.

Shift+ENTER
    Type a literal newline character.

Ctrl+LEFT and Ctrl+RIGHT
    Move cursor to previous/next word.

UP and DOWN
    Navigate command history.

PGUP
    Go to the first command in the history.

PGDN
    Stop navigating command history.

INSERT
    Toggle insert mode.

Shift+INSERT
    Paste text (uses the primary selection on X11 and Wayland).

TAB
    Complete the command or property name at the cursor.

Ctrl+C
    Clear current line.

Ctrl+K
    Delete text from the cursor to the end of the line.

Ctrl+L
    Clear all log messages from the console.

Ctrl+U
    Delete text from the cursor to the beginning of the line.

Ctrl+V
    Paste text (uses the clipboard on X11 and Wayland).

Ctrl+W
    Delete text from the cursor to the beginning of the current word.

Commands
--------

``script-message-to console type <text> [<cursor_pos>]``
    Show the console and pre-fill it with the provided text, optionally
    specifying the initial cursor position as a positive integer starting from
    1.

    .. admonition:: Example for input.conf

        ``% script-message-to console type "seek  absolute-percent" 6``

Known issues
------------

- Pasting text is slow on Windows
- Non-ASCII keyboard input has restrictions
- The cursor keys move between Unicode code-points, not grapheme clusters

Configuration
-------------

This script can be customized through a config file ``script-opts/console.conf``
placed in mpv's user directory and through the ``--script-opts`` command-line
option. The configuration syntax is described in `ON SCREEN CONTROLLER`_.

Key bindings can be changed in a standard way, see for example stats.lua
documentation.

Configurable Options
~~~~~~~~~~~~~~~~~~~~

``scale``
    Default: 1

    All drawing is scaled by this value, including the text borders and the
    cursor.

    If the VO backend in use has HiDPI scale reporting implemented, the option
    value is scaled with the reported HiDPI scale.

``font``
    Default: unset (picks a hardcoded font depending on detected platform)

    Set the font used for the REPL and the console. This probably doesn't
    have to be a monospaced font.

``font_size``
    Default: 16

    Set the font size used for the REPL and the console. This will be
    multiplied by "scale."
