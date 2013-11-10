# INTERROBANG

**Interrobang** - *A tiny launcher menu packing a big bang syntax*

Author: Jesse McClure, Copyright 2013
License: GPLv3

## Synopsis

`interrobang [-h] [-v] [-o config-string] [-] [hushbang]`

## Description

*Interrobang* is a scriptable launcher menu with a customizable shortcut syntax and completion options. 

## Options

-	*-h*
	Print help and exit 
-	*-v*
	Print version information and exit 
-	*-o* **config-string** 
	Override a setting from the runtime configuration with a full line in quotes with a syntax matching that described below for configuration files 
-	*-*
	Read configuration file from the standard input 
-	**hushbang**
	Specify a bang syntax (see below) to use by default 

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

-	font

	Select a font as either an XLFD font string, or a string that would be matched by font-config (fc-match) 
-	geometry

	One of `top`, `bottom`, or a geometry string as `WxH+X+Y`. If *X* or *Y* are -1 the bar will be centered in that direction.
-	colors
	Provide six `#RRGGBB` color strings for foreground and background for each of normal text, option listings, and selected options. 
- border
	Select a border width in pixels and an #RRGGBB color for the boarder as `Npx #RRGGBB` 
