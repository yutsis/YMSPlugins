Select-From-Clipboard Plugin for FAR
====================================
Version @version @platform

**DISTRIBUTION**: Free, under the BSD license, see License.txt.

**BRIEF DESCRIPTION**: This is a FAR plugin allowing to "paste" selection onto
a panel from Windows Clipboard, that is, to select the files whose names are
found in Clipboard.

**INSTALLATION**: Copy ClipSel.dll to a subdirectory under your PLUGINS directory
(e.g. PLUGINS\ClipSel) and restart FAR. After restarting, press F11. A new
menu item will appear, "Select from Clipboard". It's recommended to assign
a key macro Alt-Shift-Plus to call it.

This plugin understands two main types of clipboard content.

1. Text clipboard

 The plugin does simple parsing of the file list in clipboard, assuming that
all names are separated only with line breaks and other control characters
(with codes less than 0x20). Such separators are called "default separators".

 Any other characters, e.g. space, comma etc., can be separators too if you specify
them as additional separators in the plugin's options (see below).

 Spaces are trimmed before and after file names (even if you don't define them
as separators), but it is configurable (see below). If you don't define
space as an additional separator, it may be in the middle of file name.

 If you surround a filename by quotes "", everything inside them is included
into file name regardless of plugin settings (except for default separators).

 The default settings work well with the lists produced by:
 - Ctrl-Shift-Ins key in FAR's panel;
 - copying file names from FAR's command line if you pasted them there from Ctrl-Shift-Ins list;
 - command-line command "dir/b > myfile";
 - many other commands.


2. Files from Windows Explorer

 If you select files in Windows Explorer and press Copy (Ctrl-C), this file list
will also be understood by the plugin.

 **NOTE**: such list will contain full pathnames, but if the current panel is a regular
(non-plugin) panel, only filenames are taken. If you need to "paste"
selection from such list to a filename-only plugin panel, paste this list into the FAR
editor, remove file paths and copy it back to the clipboard.



The following notes relate to both content types.

- Selection is incremental. Previously selected files are NOT unselected.
You can always press Shift-NumpadMinus before the command to unselect them all.

- The names are case-insensitive by default, so both README and readme
will be selected if any of them will be found in clipboard. Be careful
in such cases! You can enable case sensitivity in Options dialog box, but you
may need it only if you work with UNIX archives where case sensitivity is
more common.

- In case-insensitive comparison, both long file names and their short ("DOS")
alternatives are checked, i.e. you may have in your list "Program Files",
or "PROGRA~1", or "progra~1". If "Case-sensitive" is enabled, only long file
names are looked for, DOS names are ignored.

- If the name in clipboard does not contain backslashes, but the panel item
does, then only the part of the panel filename after the last backslash is compared
to the name. For example, the path of the temporary panel filename is ignored,
unless the name in clipboard contains full path.

**Note**: if you specify '\' in the "Additional Separators" list in Configuration,
the names in clipboard will always be separated from their path (if any), and
then the temporary panel path will be ignored.

  
**CONFIGURING**: FAR -> Options -> Plugins -> Select from Clipboard
opens the configuration dialog box. There are three settings there:

1. Additional Separators

 All the characters you type here will be treated as filename separators,
even if they may occur in file names. By default it's empty.
Quotes surrounding file names take precedence over this list.

2. Case-sensitive    

 Check this box if you want exact case-sensitive comparison of clipboard
contents with real file names. By default it is disabled.

3. Trim leading & trailing spaces

 If checked (default), spaces before and after file names are trimmed before
comparison, unless they are surrounded by quotes.
If not checked, they are considered parts of filenames (unless included in
"additional separators") even if there's no quotes.

--------------
Michael Yutsis  
Ramat Gan, Israel  
[yutsis@gmail.com](mailto:yutsis@gmail.com)  
http://michael-y.org/FAR  
http://plugring.farmanager.com/plugin.php?pid=18  
