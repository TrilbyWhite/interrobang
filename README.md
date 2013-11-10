# INTERROBANG

**Interrobang** - *A tiny launcher menu packing a big bang syntax*

Author: Jesse McClure, Copyright 2013
License: GPLv3

## Synopsis

`interrobang [-h] [-v] [-o config-string] [-] [hushbang]`

## Description

*Interrobang* is a scriptable launcher menu with a customizable shortcut syntax and completion options. 

## Options

<dl>
<dt>-h</dt>
	<dd>Print help and exit</dd>
<dt>-v</dt>
	<dd>Print version information and exit</dd>
<dt>-o <i>config-string</i></dt>
	<dd>Override a setting from the runtime configuration with a full line in quotes with a syntax matching that described below for configuration files</dd>
<dt>-</dt>
	<dd>Read configuration file from the standard input</dd>
<dt><i>hushbang</i></dt>
	<dd>Specify a bang syntax (see below) to use by default</dd>
</dl>

## Configuration

Runtime configuration is read from the following files, in order, stopping at the first file found: 
*stdin* if *-* is specified on the command line, 
`$XDG_CONFIG_HOME/interrobang/config`,
`$HOME/.interrobangrc`.

A template configuration file is distributed as `/usr/share/interrobang/config`

Each non-empty non-comment line in a configuration file must start with
*set*, *bang*, or *tab*.
Lines starting with a `#` are ignored as comments.

### Set

```
set parameter = setting
```


The following parameters can be set: 

<dl>
<dt>font</dt>
	<dd>Select a font as either an XLFD font string, or a string that would be matched by font-config (fc-match)</dd>
<dt>geometry</dt>
	<dd>One of `top`, `bottom`, or a geometry string as `WxH+X+Y`. If
	<i>X</i> or <i>Y</i> are -1 the bar will be centered in that direction.
<dt>colors</dt>
	<dd>Provide six `#RRGGBB` color strings for foreground and background for each of normal text, option listings, and selected options.</dd>
<dt>border</dt>
	<dd>Select a border width in pixels and an #RRGGBB color for the boarder as `Npx #RRGGBB`</dd>
<dt>bangchar</dt>
	<dd>Specify the "bang" character. ! is the default, : is a common alternative.</dd>
<dt>run_hook</dt>
	<dd>A format string into which the selected command will be placed before running. This can be used to store a history of commands. This string should contain one %s which will be replaced by the selected command. </dd>
<dt>autocomp</dt>
	<dd>Whether commands should be autocompleted. Select 0 for no autocompletion, or a positive number of characters as the minimum number required from the user before completion options are provided.</dd>
<dt>list</dt>
	<dd>Completion options displayed in bar: true or false.</dd>
<dt>last</dt>
	<dd>Last "word" only in completion options: true or false.</dd>
<dt>margin</dt>
	<dd>Number of pixels for the margin between the displayed completion options and the end of the entered text. Positive values will left-align the options at this margin, while negative values will right-align the options in the window and use this value as the minimum space between entered text and the first option.</dd>
<dt>shell</dt>
	<dd>Specify an alternative shell for launching commands</dd>
<dt>flags</dt>
	<dd>Specify alternative flags to the shell. The default -c can be replaced with -ic for an interactive shell which will allow for the use of shell rc-defined aliases.</dd>
</dl>

### Bang

```
bang keyword = command-string
```

Bangs - inspired by the duckduckgo search engine - are customizable 
commands which allow the user to extend interrobang's functionality in 
creative ways. If the first character entered in interrobang matches the 
bangchar (! by default), the word immediately following the bangchar is 
read as the bang. This bang is looked up in the configuration, and the 
remainder of the entry takes the place of the `%s` in the **command-string**.

See the default configuration for examples of how bangs can be 
customized. 

### Tab

```
tab keyword = command-string
```

For each defined bang, there can be a separate tab-completion mechanism. 
Various examples can be found in the default configuration file. The 
special keyword "default" is used for tab-completion for regular 
entries. Two default options are provided for either bash's built in 
completion with compgen, or with the percontation tool which is 
distributed with interrobang. 

**command-string** must be a format string with two `%s` entries. The 
first `%s` with all but the last "word" in the current entry, while the 
second will contain this last (potentially partial) "word". This allows 
for completion on the entire line, or based just on the word being 
typed.

See the default configuration for examples of how tab completion can be 
customized. 

## Author

Copyright &copy;2013 Jesse McClure

License GPLv3: GNU GPL version 3: http://gnu.org/licenses/gpl.html

This is free software: you are free to change and redistribute it.

There is NO WARRANTY, to the extent permitted by law. 

Submit bug reports via github: http://github/com/TrilbyWhite/interrobang.git

I would like your feedback. If you enjoy *Interrobang*
see the bottom of the site below for detauls on submitting comments: http://mccluresk9.com/software.html

