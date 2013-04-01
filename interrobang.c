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
#include <locale.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#define MAX_LINE	240

typedef struct Bang {
	char *bang;
	char *command;
	char *comp;
} Bang;

static Bang *bangs;
static int nbangs, scr, fh, x = 0, y = 0, w = 0, h = 0;
static Display *dpy;
static Window root, win;
static Pixmap buf;
static GC gc, bgc;
static XFontSet xfs;
static XIC xic;
static char bangchar = '!', colBG[8] = "#121212", colFG[8] = "#EEEEEE", colBD[8] = "";
static char font[MAX_LINE] = "-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
static char line[MAX_LINE+4],bang[MAX_LINE],cmd[2*MAX_LINE], completion[MAX_LINE];
static char defaultcomp[MAX_LINE] = "";

static int config(int argc, const char **argv) {
	FILE *rc; char *c; int i;
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '\0') rc = stdin;
	else { chdir(getenv("HOME")); rc = fopen(".interrobangrc","r"); }
	if (!rc) return -1;
	while (fgets(line,MAX_LINE,rc) != NULL) {
		if (line[0] == '#' || line[0] == '\n') continue;
		else if (line[0] == bangchar && line[1] != '\0') {
			sscanf(line+1,"%s %[^\n]",bang,cmd);
			bangs = (Bang *) realloc(bangs,(nbangs + 1) * sizeof(Bang));
			bangs[nbangs].bang = strdup(bang);
			bangs[nbangs++].command = strdup(cmd);
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
			if (strlen(c) > 4) strncpy(font,c,strlen(c)-1);
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

static int init_X() {
	/* locale, open connection, basic info */
	if ( !(setlocale(LC_CTYPE,"") && XSupportsLocale()) ) exit(-1);
	if (XSetLocaleModifiers("") == NULL) exit(-1);
	if (!(dpy=XOpenDisplay(0x0))) exit(1);
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	w = (w ? w : DisplayWidth(dpy,scr) - (strlen(colBD)?2:0));
	Colormap cmap = DefaultColormap(dpy,scr);
	XColor color; XGCValues val;
	/* fonts */
	char **missing, **names, *def; int nmiss, i;
	xfs = XCreateFontSet(dpy,font,&missing,&nmiss,&def);
	if (!xfs) exit(-1);
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
	fh = fss[0]->ascent + 1;
	if (!h) h = fh + fss[0]->descent + 2;
	if (y == -1) y = DisplayHeight(dpy,scr) - h;
	/* grab keys */
	for (i = 0; i < 1000; i++) {
		if (XGrabKeyboard(dpy,root,True,GrabModeAsync,GrabModeAsync,
			CurrentTime) == GrabSuccess) break;
		usleep(1000);
	}
	if (i == 1000) exit(1);
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
	xic = XCreateIC(xim,XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
			XNClientWindow, win, XNFocusWindow, win, NULL);
	XMapWindow(dpy,win);
	XFillRectangle(dpy,buf,bgc,0,0,w,h);
	XDrawLine(dpy,buf,gc,5,2,5,fh);
	XCopyArea(dpy,buf,win,gc,0,0,w,h,0,0);
	XFlush(dpy);
	return 0;
}

static int main_loop() {
	XEvent ev; XKeyEvent *e; KeySym key;
	int breakcode = 0, x = 0, i, compcount = 0, compcur = 0, len = 0;
	unsigned int mod;
	char prefix[MAX_LINE+3], *sp = NULL;
	char **complist = NULL, txt[32], *c, *comp = NULL;
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
			if (key == 'u') line[0] = '\0';
		}
		else if (key == XK_Return) breakcode = 1;
		else if (key == XK_Escape) breakcode = -1;
		else if (key == XK_Delete) line[0] = '\0';
		else if (key == XK_space) strcat(line," ");
		else if (key == XK_BackSpace) {
			for (c = &line[strlen(line)-1];(*c&0xC0)==0x80; c--);
			*c = '\0';
		}
		else if (key == XK_Tab) {
			if (!compcheck) {
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
				if (line[0] == bangchar && line[1] != '\0') {
					for (i = 0; i < nbangs; i++)
						if (strncmp(bangs[i].bang,line+1,strlen(bangs[i].bang))==0)
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
			}
			if (compcheck) {
				if (mod & ShiftMask) {
					if ((--compcur) < 0 ) compcur = compcount - 1;
				}
				else if ( (++compcur) >= compcount ) compcur = 0;
				strcpy(line,complist[compcur]);
			}
		}
		else {
			if (!iscntrl(*txt)) strncat(line,txt,len);
		}
		if (key != XK_Tab && key != XK_Shift_L && key != XK_Shift_R) compcheck = False;
		/* draw */
		XFillRectangle(dpy,buf,bgc,0,0,w,h);
		XmbDrawString(dpy,buf,xfs,gc,5,fh,line,strlen(line));
		x = XmbTextEscapement(xfs,line,strlen(line));
		XDrawLine(dpy,buf,gc,x+5,2,x+5,fh);
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

static int process_command() {
	int i, x = 0; char *c, *b = NULL;
	strcpy(cmd,"");
	if (line[0] == bangchar) { /* "bang" syntax: */
		if ( (c=strchr(line,' ')) != NULL) x = c - line - 1; /* x = length of bang */
		else x = strlen(line + 1);
		if (x) for (i = 0; i < nbangs; i++)	/* b = bang command */
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
		strcpy(cmd,line);
	} 
	strcat(cmd," &");
	if (strlen(cmd) > 2) system(cmd);
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

int main(int argc, const char **argv) {	
	config(argc,argv);
	init_X();
	if (main_loop())
		process_command();
	clean_up();
	return 0;
}

