# A minimal and embeddable text editor

This is by far the most minialistic text editor I am aware of. It is inspired by vi.

After looking unsuccessfully for a minimalistic text editor, I decided to write my own.

It is implemented in such a way that it can easily be embedded into other programs.

**NOTE: There is NO UNDO in this editor. Use ':rl' to reload the file from disk.**

There are 4 modes in this editor:

- NORMAL
- INSERT
- REPLACE
- COMMAND

## These control keys are active in all modes:

| Key      | Action |
| :--      | :-- |
| [ctrl]+h | Left 1 char (Delete if in INSERT mode) |
| [ctrl]+j | Down 1 line |
| [ctrl]+k | Up 1 line |
| [ctrl]+l | Right 1 char |
| [tab]    | Right 8 chars |
| [ctrl]+i | Right 8 chars |
| [ctrl]+e | Scroll down 1 line |
| [ctrl]+y | Scroll up 1 line |
| [ctrl]+d | Scroll down 1/2 screen |
| [ctrl]+u | Scroll up 1/2 screen |
| [ctrl]+x | Delete the char to the left of the cursor |
| [ctrl]+z | Delete the char under the cursor |
| [escape] | Change to NORMAL mode |

### NORMAL mode

NORMAL mode is similar to VI:

| Key  | Action|
| :--  | :-- |
| h    | Left 1 char |
| j    | Down 1 line |
| k    | Up 1 line |
| l    | Right 1 char |
| _    | Goto the beginning of the line |
| $    | Goto the end of the line |
| +    | Increase the number of lines shown |
| -    | Decrease the number of lines shown |
| [SP] | Right 1 char |
| [CR] | Goto the beginning of the next line |
| a    | Append: move right 1 char and change to INSERT mode |
| A    | Append: goto the end of the line and change to INSERT mode |
| b    | Insert a single space at he cursor |
| c    | Change: Delete the current char and change to INSERT mode (same as 'xi') |
| C    | Change: Delete to the end of the line and change to INSERT mode (same as 'd$A') |
| dd   | Copy the current line into the YANK buffer and delete the line |
| d.   | Delete the char under the cursor (same as 'x') |
| dw   | Delete to the end of the word |
| d$   | Delete to the end of the line |
| D    | Delete to the end of the line (same as 'd$') |
| g    | Goto the top-left of the screen |
| G    | Goto the bottom-left of the screen |
| i    | Insert: change to INSERT mode |
| I    | Insert: goto the beginning of the line and change to INSERT mode |
| J    | Join the current and next lines together |
| o    | Insert an empty line BELOW the current line and change to INSERT mode |
| O    | Insert an empty line ABOVE the current line and change to INSERT mode |
| p    | Paste the YANK buffer into a new line BELOW the current line |
| P    | Paste the YANK buffer into a new line ABOVE the current line |
| r    | Replace the char under the cursor with the next key pressed (if printable) |
| R    | Change to REPLACE mode |
| x    | Delete the char under the cursor |
| X    | Delete the char to the left of the cursor |
| Y    | Copy the current line into the YANK buffer |
| :    | Change to COMMAND mode |

### INSERT mode

In INSERT mode, all printable characters are inserted into the file.

Carriage-Return inserts a new line.

### REPLACE mode

In REPLACE mode, all printable characters are placed into the file.

Carriage-Return moves the cursor to the beginning of the next line.

### COMMAND mode

COMMAND mode is invoked when pressing ':' in NORMAL mode.

| Command | Action|
| :--     | :-- |
| w       | Write the current file if it has changed |
| w!      | Write the current file, even if it has NOT changed |
| q       | Quit, if the current file has NOT changed |
| q!      | Quit, even if the current file has changed |
| wq      | Write the current file and quit (same as ':w' ':q') |
| rl      | Reload the current file. |
