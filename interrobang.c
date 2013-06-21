/**************************************************************************\
* INTERROBANG - a tiny launcher menu packing a big bang (syntax)
*
* Author: Jesse McClure, copyright 2013
* License: GPLv3
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
\**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#define VERSION_STRING \
	"INTERROBANG, version 0.1\n" \
	"Copyright (C) 2013 Jesse McClure\n" \
	"License GPLv3 <http://gnu.org/licenses/gpl.html>\n"

#define HELP_STRING \
	"\nUsage: %s [option] [hushbang]\n\n" \
	"Options:\n\t-h\tShow this help message and exit.\n" \
	"\t-v\tShow version information and exit.\n" \
	"\t-\tOverride ~/.interrobangrc with configuration read from stdin\n\n" \
	"Hushbang:\n\tProvide any bang (without bangchar) to have the associated\n" \
	"\tcommmand executed on the input string\n\n" \
	"See `man interrobang` for more information. (NOT YET AVAILABLE)\n"

#define MAX_LINE	240

typedef struct Bang {
	char *bang;
	char *command;
	char *comp;
} Bang;

static Bang *bangs;
static int nbangs, scr, fh, x = 0, y = 0, w = 0, h = 0, hushbang = -1;
static int precomp = 0;
static Display *dpy;
static Window root, win;
static Pixmap buf;
static GC gc, bgc, ogc, osgc;
static XFontSet xfs;
static XIC xic;
static char bangchar = '!', colBG[8] = "#121212", colFG[8] = "#EEEEEE", colBD[8] = "";
static char font[MAX_LINE] =
		"-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
static char line[MAX_LINE+4],bang[MAX_LINE],cmd[2*MAX_LINE], completion[MAX_LINE];
static char defaultcomp[MAX_LINE] = "";
static Bool show_opts = False, autocomplete = False;
static char opt_col[4][8] = {"#242424","#48E084","#484848","#64FFAA"};

static int config(int argc, const char **argv) {
	FILE *rc=NULL; char *c; int i;
	const char *hushbangstr = NULL, *cwd = getenv("PWD");
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			char flag = (argv[i][1] == '-' ? argv[i][2] : argv[i][1]);
			if (flag == '\0') rc = stdin;
			else if (flag == 'v') { printf(VERSION_STRING); exit(0); }
			else if (flag == 'h') {
				printf(VERSION_STRING HELP_STRING,argv[0]);
				exit(0);
			}
			else fprintf(stderr,"unrecognized parameter \"%s\"\n",argv[i]);
		}
		else hushbangstr = argv[i];
	}
	if (!rc) {
		chdir(getenv("XDG_CONFIG_HOME"));
		if (chdir("interrobang")==0) rc = fopen("config","r");
	}
	if (!rc) { chdir(getenv("HOME")); rc = fopen(".interrobangrc","r"); }
	chdir(cwd);
	if (!rc) return -1;
	while (fgets(line,MAX_LINE,rc) != NULL) {
		if (line[0] == '#' || line[0] == '\n') continue;
		else if (line[0] == bangchar && line[1] != '\0') {
			sscanf(line+1,"%s %[^\n]",bang,cmd);
			if (hushbangstr && strncmp(bang,hushbangstr,strlen(bang))==0)
				hushbang = nbangs;
			bangs = (Bang *) realloc(bangs,(nbangs + 1) * sizeof(Bang));
			bangs[nbangs].bang = strdup(bang);
			bangs[nbangs++].command = strdup(cmd);
		}
		else if (strncmp(line,"show",4)==0) {
			for (c = line + 4; *c == ' ' || *c == '\t'; c++);
			if (*c == 'o') show_opts = True;
		}
		else if (strncmp(line,"autocomplete",11)==0) {
			autocomplete = True;
		}
		else if (strncmp(line,"options",7)==0) {
			sscanf(line,"options %7s %7s %7s %7s",
					opt_col[0],opt_col[1],opt_col[2],opt_col[3]);
		}
		else if (strncmp(line,"background",10)==0) {
			if ( (c=strchr(line,'#')) && strlen(c) > 6) strncpy(colBG,c,7);
		}
		else if (strncmp(line,"foreground",10)==0) {
			if ( (c=strchr(line,'#')) && strlen(c) > 6) strncpy(colFG,c,7);
		}
		else if (strncmp(line,"border",6)==0) {
			if ( (c=strchr(line,'#')) && strlen(c) > 6) strncpy(colBD,c,7);
		}
		else if (strncmp(line,"font",4)==0) {
			for (c = line + 4; *c == ' ' || *c == '\t'; c++);
			if (strlen(c) > 4) strncpy(font,c,strlen(c)+1);
		}
		else if (strncmp(line,"bangchar",8)==0) {
			for (c = line + 8; *c == ' ' || *c == '\t'; c++);
			if (*c != '\n' && *c != '\0') bangchar = *c;
		}
		else if (strncmp(line,"TAB(",4)==0) {
			if (line[4] == ')') {
				for (c = line + 5; *c == ' ' || *c == '\t'; c++);
				if (strlen(c) > 4) strncpy(defaultcomp,c,strlen(c)-1);
			}
			else {
				sscanf(line,"TAB(%[^)])%*[ \t]%[^\n]",bang,cmd);
				for (i = 0; i < nbangs; i++)
					if (strncmp(bangs[i].bang,bang,strlen(bang))==0)
						bangs[i].comp = strdup(cmd);
			}
		}
		else if (strncmp(line,"geometry",8)==0) {
			for (c = line + 8; *c == ' ' || *c == '\t'; c++);
			if (*c != '\n' && *c != '\0') {
				if (*c == 'b') y = -1;
				else sscanf(c,"%dx%d+%d+%d",&w,&h,&x,&y);
			}
		}
		else {
			line[strlen(line)-1] = '\0';
			fprintf(stderr,"unrecognized configuration entry \"%s\"\n",line);
		}
	}
	if (rc != stdin) fclose(rc);
	strcpy(line,"");
	return 0;
}

static int die(const char *msg,...) {
	fprintf(stderr,"INTERROBANG: Error ");
	va_list arg; va_start(arg,msg);
	vfprintf(stderr,msg,arg);
	va_end(arg);
	exit(1);
}

static int init_X() {
	/* locale, open connection, basic info */
	if ( !(setlocale(LC_CTYPE,"") && XSupportsLocale()) )
			die("setting locale\n");
	if (XSetLocaleModifiers("") == NULL) die("setting modifiers\n");
	if (!(dpy=XOpenDisplay(0x0))) die("opening display\n");
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	w = (w ? w : DisplayWidth(dpy,scr) - (strlen(colBD)?2:0));
	Colormap cmap = DefaultColormap(dpy,scr);
	XColor color; XGCValues val;
	/* fonts */
	char **missing, **names, *def; int nmiss, i;
	xfs = XCreateFontSet(dpy,font,&missing,&nmiss,&def);
	if (!xfs) die("loading font \"%s\"\n",font);
	XFontStruct **fss;
	XFontsOfFontSet(xfs,&fss,&names);
	if (missing) XFreeStringList(missing);
	/* graphic contexts */
	XAllocNamedColor(dpy,cmap,colBG,&color,&color);
	val.foreground = color.pixel;
	bgc = XCreateGC(dpy,root,GCForeground,&val);
	XAllocNamedColor(dpy,cmap,colFG,&color,&color);
	val.foreground = color.pixel;
	gc = XCreateGC(dpy,root,GCForeground,&val);
	XAllocNamedColor(dpy,cmap,opt_col[0],&color,&color);
	val.background = color.pixel;
	XAllocNamedColor(dpy,cmap,opt_col[1],&color,&color);
	val.foreground = color.pixel;
	ogc = XCreateGC(dpy,root,GCForeground|GCBackground,&val);
	XAllocNamedColor(dpy,cmap,opt_col[2],&color,&color);
	val.background = color.pixel;
	XAllocNamedColor(dpy,cmap,opt_col[3],&color,&color);
	val.foreground = color.pixel;
	osgc = XCreateGC(dpy,root,GCForeground|GCBackground,&val);
	fh = fss[0]->ascent + 1;
	if (!h) h = fh + fss[0]->descent + 2;
	if (y == -1) y = DisplayHeight(dpy,scr) - h;
	/* grab keys */
	for (i = 0; i < 1000; i++) {
		if (XGrabKeyboard(dpy,root,True,GrabModeAsync,GrabModeAsync,
			CurrentTime) == GrabSuccess) break;
		usleep(1000);
	}
	if (i == 1000) die("grabbing keyboard\n");
	/* create window and buffer */
	XSetWindowAttributes wa;
	wa.override_redirect = True;
	wa.border_pixel = (XAllocNamedColor(dpy,cmap,colBD,&color,&color) ?
		color.pixel : 0);
	win = XCreateWindow(dpy,root,x,y,w,h,(strlen(colBD)?1:0),
		DefaultDepth(dpy,scr),CopyFromParent,DefaultVisual(dpy,scr),
		CWOverrideRedirect|CWBorderPixel,&wa);
	buf = XCreatePixmap(dpy,root,w,h,DefaultDepth(dpy,scr));
	/* input context */
	XIM xim = XOpenIM(dpy,NULL,NULL,NULL);
	if (!xim) die("No X input method could be opened\n");
	xic = XCreateIC(xim,XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
			XNClientWindow, win, XNFocusWindow, win, NULL);
	XMapWindow(dpy,win);
	XFillRectangle(dpy,buf,bgc,0,0,w,h);
	XDrawLine(dpy,buf,gc,5,2,5,fh);
	XCopyArea(dpy,buf,win,gc,0,0,w,h,0,0);
	XFlush(dpy);
	if (autocomplete) {
		XKeyEvent e;
		e.display = dpy; e.window = root; e.root = root; e.subwindow = None;
		e.time = CurrentTime; e.type = KeyPress; e.state = 0;
		e.keycode = XKeysymToKeycode(dpy,XK_Tab);
		XPutBackEvent(dpy,(XEvent *) &e); XFlush(dpy);
	}
	return 0;
}

static int options(int n, const char **opt, int cur, int x) {
	int i, j, wx, tx;
	tx = XmbTextEscapement(xfs,">",1);
	for (j = n - 1; j; j--) {
		for (wx = w, i = j; wx > x+tx && i > -1; i--) {
			wx -= XmbTextEscapement(xfs,opt[i],strlen(opt[i])) +
					XmbTextEscapement(xfs," ",1);
		}
		if (i < cur) break;
	}
	wx = w;
	if (j - i < n) XmbDrawImageString(dpy,buf,xfs,ogc,(wx-=tx),fh,">",1);
	for (i = j; wx > x && i >= 0; i--) {
		tx = XmbTextEscapement(xfs,opt[i],strlen(opt[i]));
		XmbDrawImageString(dpy,buf,xfs,(i==cur?osgc:ogc),(wx-=tx),
				fh,opt[i],strlen(opt[i]));
		tx = XmbTextEscapement(xfs," ",1);
		XmbDrawImageString(dpy,buf,xfs,(i==cur?osgc:ogc),(wx-=tx),fh," ",1);
	}
}

static int main_loop() {
	XEvent ev; XKeyEvent *e; KeySym key;
	int breakcode = 0, tx = 0, i, compcount = 0, compcur = 0, len = 0, pos = 0;
	char prefix[MAX_LINE+3], *sp = NULL;
	char **complist = NULL, txt[32], *c, *comp = NULL, *part;
	FILE *compgen; Bool compcheck = False; Status stat;
	while (!XNextEvent(dpy,&ev)) {
		if (XFilterEvent(&ev,win)) continue;
		if (ev.type != KeyPress) continue;
		/* get key */
		e = &ev.xkey;
		key = NoSymbol;
		len = XmbLookupString(xic,e,txt,sizeof txt,&key,&stat);
		if (stat == XBufferOverflow) continue;
		if (e->state & Mod1Mask) continue;
		if (e->state & ControlMask) {
			if (key == 'u') line[(pos=0)] = '\0';
			if (key == 'c') line[(pos=precomp)] = '\0';
		}
		else if (key == XK_Return) breakcode = 1;
		else if (key == XK_Escape) breakcode = -1;
		else if (key == XK_Delete && line[pos] != '\0') {
			part = &line[pos];
			for (c = &line[(++pos)];(*c&0xC0)==0x80; c++, pos++);
			strcpy(part,c);
			pos = part-line;
			compcheck = False;
		}
		else if (key == XK_BackSpace && pos > 0) {
			part = strdup(&line[pos]);
			for (c = &line[(--pos)];(*c&0xC0)==0x80; c--, pos--);
			strcpy(c,part); free(part);
			compcheck = False;
		}
		else if (key == XK_Left && pos > 0) pos--;
		else if (key == XK_Right && pos < strlen(line)) pos++;
		else if (key == XK_Insert && (e->state & ShiftMask)) {
			Window w;
			int fmt, res;
			unsigned long len, rem;
			unsigned char *s = NULL;
			XEvent e;
			Atom type;
			Window sel = XGetSelectionOwner(dpy, XA_PRIMARY);
			if (sel) {
				XConvertSelection(dpy,XA_PRIMARY,XA_STRING,None,sel,
						CurrentTime);
				XFlush(dpy);
				XMaskEvent(dpy,SelectionNotify,&e);
				XGetWindowProperty(dpy,sel,XA_STRING,0,256,False,
						AnyPropertyType,
						&type, &fmt, &len, &rem, &s);
				if (s) {
					strcat(line,s);
					XFree(s);
				}
			}
			compcheck = False;
		}
		else if (key == XK_Tab || key == XK_ISO_Left_Tab || 
				key == XK_Down || key == XK_Up) {
			if (!compcheck) {
				precomp = strlen(line);
				if (complist) {
					for (i = 0; i < compcount; i++) free(complist[i]);
					free(complist);
					complist = NULL;
					compcount = 0;
				}
				if ( (sp=strrchr(line,' ')) ) {
					sp++;
					strcpy(prefix,line);
					prefix[sp-line] = '\0';
				}
				else {
					sp = line;
					prefix[0] = '\0';
				}
				comp = NULL;
				if (hushbang > -1) {
					comp = bangs[hushbang].comp;
				}
				else if (line[0] == bangchar && line[1] != '\0') {
					for (i = 0; i < nbangs; i++)
						if (strncmp(bangs[i].bang,line+1,
								strlen(bangs[i].bang))==0)
							comp = bangs[i].comp;
				}
				if (!comp) comp = defaultcomp;
				sprintf(cmd,comp,prefix,sp);
				compgen = popen(cmd,"r");
				while (fgets(cmd,MAX_LINE,compgen) != NULL) {
					if (strlen(cmd) < 4) continue;
					cmd[strlen(cmd) - 1] = '\0'; /* trim newlines */
					compcount++;
					complist =
						(char **) realloc(complist,compcount*sizeof(char *));
					complist[compcount - 1] = strdup(cmd);
				}
				pclose(compgen);
				if (complist) compcheck = True;
				compcur = -1;
			}
			if (compcheck) {
				if ( key==XK_ISO_Left_Tab || key == XK_Up ) {
					if ((--compcur) < 0 ) compcur = compcount - 1;
				}
				else if ( (++compcur) >= compcount ) compcur = 0;
				strcpy(line,complist[compcur]);
			}
		}
		else {
			if (!iscntrl(*txt)) {
				part = strdup(&line[pos]);
				line[pos] = '\0';
				strncat(line,txt,len);
				strcat(line,part); free(part);
				pos+=len;
				compcheck = False;
			}
		}
		/* draw */
		XFillRectangle(dpy,buf,bgc,0,0,w,h);
		XmbDrawString(dpy,buf,xfs,gc,5,fh,line,strlen(line));
		tx = XmbTextEscapement(xfs,line,pos);
		XDrawLine(dpy,buf,gc,tx+5,2,tx+5,fh);
		if (show_opts && compcheck)
			options(compcount,(const char **)complist,compcur,tx + 100);
		XCopyArea(dpy,buf,win,gc,0,0,w,h,0,0);
		XFlush(dpy);
		if (breakcode) break;
	}
	if (complist) {
		for (i = 0; i < compcount; i++) free(complist[i]);
		free(complist);
		complist = NULL;
		compcount = 0;
	}
	XUngrabKeyboard(dpy,CurrentTime);
	return (breakcode == 1 ? 1 : 0);
}

static int clean_up() {
	XFreeFontSet(dpy,xfs);
	XFreeGC(dpy,bgc); XFreeGC(dpy,gc);
	XDestroyIC(xic);
	XFreePixmap(dpy,buf);
	XDestroyWindow(dpy,win);
	XCloseDisplay(dpy);
	return 0;
}

static int process_command() {
	int i, x = 0; char *c, *b = NULL;
	strcpy(cmd,"");
	if (line[0] == bangchar) { /* "bang" syntax: */
		/* x = length of bang */
		if ( (c=strchr(line,' ')) != NULL) x = c - line - 1;
		else x = strlen(line + 1);
		/* b = bang command */
		if (x) for (i = 0; i < nbangs; i++)
			if (strncmp(bangs[i].bang,line + 1,x) == 0)
				b = bangs[i].command;
		if (!b && nbangs != 0) {	/* use default bang */
			b = bangs[0].command;
			c = line;
		}
		if (b && c) { c++; sprintf(cmd,b,c); }
		else if (b)	sprintf(cmd,b, line + 1);
	}
	else {
		if (hushbang > -1) sprintf(cmd,bangs[hushbang].command,line);
		else strcpy(cmd,line);
	} 
	clean_up();
	if (strlen(cmd) > 2) {
		const char *argv[4]; argv[0] = "/bin/sh"; argv[1] = "-c";
		argv[3] = NULL; argv[2] = cmd;
		execv(argv[0],argv);
	}
}

int main(int argc, const char **argv) {	
	config(argc,argv);
	init_X();
	if (main_loop()) process_command();
	else clean_up();
	return 0;
}

