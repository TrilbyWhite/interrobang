/************************************************************************\
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
\************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#define VERSION_STRING \
	"INTERROBANG, version 0.2\n" \
	"Copyright (C) 2013 Jesse McClure\n" \
	"License GPLv3 <http://gnu.org/licenses/gpl.html>\n"

#define HELP_STRING \
	"\nUsage: %s [option] [hushbang]\n\n" \
	"Options:\n" \
	"   -h         Show this help message and exit.\n" \
	"   -v         Show version information and exit.\n" \
	"   -o <opt>   Override a single setting from rc file\n" \
	"   -          Override ~/.interrobangrc with configuration " \
			"read from stdin\n\n" \
	"Hushbang:\n" \
	"   Provide any bang (without bangchar) to have the\n" \
	"   associated commmand executed on the input string\n\n" \
	"See `man interrobang` for more information. (NOT YET AVAILABLE)\n"

#define MAX_LINE	240

typedef struct Bang {
	char *bang;
	char *command;
	char *comp;
} Bang;

static Display *dpy;
static int nbangs,scr,fh,x=0,y=0,w=0,h=0,hush=-1,autocomp=-1,bpx=0,margin=-80;
static Window root, win;
static Pixmap buf;
static GC gc, bgc, ogc, osgc;
static XFontSet xfs;
static XIC xic;
static Bang *bangs;
static Bool show_opts = False, last_word = False;
static const char *hushstr = NULL;
static char bangchar = '!', font[MAX_LINE] = "fixed",
		line[MAX_LINE+4], bang[MAX_LINE], cmd[2*MAX_LINE],
		completion[MAX_LINE], defaultcomp[MAX_LINE] = "",
		shell_flags[10] = "-c", *run_hook = NULL;
static char col[7][8] = {
	/* BG		FG			BRD */
	"#121212", "#EEEEEE", "#000000",
	"#242424", "#48E084", 
	"#484848","#64FFAA"
};

static int config_string(const char *str) {
	int i; char cmd[12], opt[32], val[MAX_LINE];
	sscanf(str,"%s %s = %[^\n]",cmd,opt,val);
	if (strncmp(cmd,"set",3)==0) {
		if (strncmp(opt,"font",4)==0) strncpy(font,val,MAX_LINE-1);
		else if (strncmp(opt,"geom",4)==0) {
			if (strncmp(val,"bot",3)==0) y = -2;
			else sscanf(val,"%dx%d%d%d",&w,&h,&x,&y);
		}
		else if (strncmp(opt,"col",3)==0)
			sscanf(val,"%s %s %s %s %s %s",col[0],col[1],
					col[3],col[4],col[5],col[6]);
		else if (strncmp(opt,"bord",4)==0)
			sscanf(val,"%dpx %s",&bpx,col[2]);
		else if (strncmp(opt,"bang",4)==0)
			bangchar = val[0];
		else if (strncmp(opt,"run",3)==0)
			run_hook = strdup(val);
		else if (strncmp(opt,"shell",5)==0)
			strncpy(shell_flags,val,9);
		else if (strncmp(opt,"auto",4)==0)
			autocomp = atoi(val);
		else if (strncmp(opt,"margin",6)==0)
			margin = atoi(val);
		else if (strncmp(opt,"list",4)==0)
			show_opts = (val[0]=='T'||val[0]=='t');
		else if (strncmp(opt,"last",4)==0)
			last_word = (val[0]=='T'||val[0]=='t');
		else printf("interrobang: unknown option \"%s\"\n",opt);
	}
	else if (strncmp(cmd,"bang",4)==0) {
		if (hushstr && strncmp(opt,hushstr,strlen(opt))==0)
			hush = nbangs;
		bangs = (Bang *) realloc(bangs,(nbangs + 1) * sizeof(Bang));
		bangs[nbangs].bang = strdup(opt);
		bangs[nbangs++].command = strdup(val);
	}
	else if (strncmp(cmd,"tab",3)==0) {
		if (strncmp(opt,"default",7)==0)
			strncpy(defaultcomp,val,MAX_LINE-1);
		else for (i = 0; i < nbangs; i++)
			if (strncmp(bangs[i].bang,opt,strlen(opt))==0)
				bangs[i].comp = strdup(val);
	}
	else {
		printf("interrobang: unknown config command\"%s\"\n",cmd);
	}
}

static int config(int argc, const char **argv) {
	FILE *rc=NULL; char *c; int i;
	const char *cwd = getenv("PWD");
	/* read command line, ignoring "-opt" settings */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			char flag = (argv[i][1] == '-' ? argv[i][2] : argv[i][1]);
			if (flag == '\0') rc = stdin;
			else if (flag == 'v') { printf(VERSION_STRING); exit(0); }
			else if (flag == 'h') {
				printf(VERSION_STRING HELP_STRING,argv[0]);
				exit(0);
			}
			else if (flag == 'o' && argc > i + 1) i++;
			else fprintf(stderr,"unrecognized parameter \"%s\"\n",argv[i]);
		}
		else hushstr = argv[i];
	}
	/* open and read rc file */
	if (!rc && chdir(getenv("XDG_CONFIG_HOME"))==0 && chdir("interrobang")==0 )
		rc = fopen("config","r");
	if (!rc && chdir(getenv("HOME"))==0 && chdir(".config/interrobang")==0 )
		rc = fopen("config","r");
	if (!rc && chdir(getenv("HOME"))==0 )
		rc = fopen(".interrobangrc","r");
	chdir(cwd);
	if (!rc) return -1;
	while (fgets(line,MAX_LINE,rc) != NULL) {
		if (line[0] == '#' || line[0] == '\n') continue;
		config_string(line);
	}
	if (rc != stdin) fclose(rc);
	/* check command line for option overrides */
	for (i = 1; i < argc - 1; i++)
		if (argv[i][0]=='-' && argv[i][1]=='o') config_string(argv[i+1]);
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
	w = (w ? w : DisplayWidth(dpy,scr) - bpx*2);
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
	XAllocNamedColor(dpy,cmap,col[1],&color,&color);
	val.foreground = color.pixel;
	bgc = XCreateGC(dpy,root,GCForeground,&val);
	XAllocNamedColor(dpy,cmap,col[0],&color,&color);
	val.foreground = color.pixel;
	gc = XCreateGC(dpy,root,GCForeground,&val);
	XAllocNamedColor(dpy,cmap,col[4],&color,&color);
	val.background = color.pixel;
	XAllocNamedColor(dpy,cmap,col[3],&color,&color);
	val.foreground = color.pixel;
	ogc = XCreateGC(dpy,root,GCForeground|GCBackground,&val);
	XAllocNamedColor(dpy,cmap,col[6],&color,&color);
	val.background = color.pixel;
	XAllocNamedColor(dpy,cmap,col[5],&color,&color);
	val.foreground = color.pixel;
	osgc = XCreateGC(dpy,root,GCForeground|GCBackground,&val);
	fh = fss[0]->ascent;
	if (!h) h = fh + fss[0]->descent;
	if (y == -2) y = DisplayHeight(dpy,scr) - h;
	if (y == -1) y = (DisplayHeight(dpy,scr) - h)/2;
	if (x == -1) x = (DisplayWidth(dpy,scr) - w)/2;
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
	wa.border_pixel = (XAllocNamedColor(dpy,cmap,col[2],&color,&color) ?
		color.pixel : 0);
	win = XCreateWindow(dpy,root,x,y,w,h,bpx,DefaultDepth(dpy,scr),
			CopyFromParent,DefaultVisual(dpy,scr),
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
	if (autocomp == 0) {
		XKeyEvent e;
		e.display = dpy; e.window = root; e.root = root; e.subwindow = None;
		e.time = CurrentTime; e.type = KeyPress; e.state = 0;
		e.keycode = XKeysymToKeycode(dpy,XK_Tab);
		XPutBackEvent(dpy,(XEvent *) &e); XFlush(dpy);
	}
	return 0;
}

static int options(int n,const char **opt, int cur, int x) {
	x+= abs(margin);
	int i, j, k, wx, m, tx = XmbTextEscapement(xfs,">",1);
	const char *lw;
	int *offset = (int *) calloc(n,sizeof(int));
	for (j = 0; j < n; j++) {
		wx = x+tx;
		for (i = j; i < n && wx < w; i++) {
			lw = strrchr(opt[i],' ');
			if (!lw || *(++lw) == '\0' || !last_word) lw = opt[i];
			offset[i] = wx;
			wx += XmbTextEscapement(xfs,lw,strlen(lw))+XmbTextEscapement(xfs," ",1);
		}
		if (i > cur) break;
	}
	m = (margin < 0 ? w - wx : 0);
	for (k = j; k < i; k++) {
		lw = strrchr(opt[k],' ');
		if (!lw || *(++lw) == '\0' || !last_word) lw = opt[k];
		XmbDrawImageString(dpy,buf,xfs,(k==cur?osgc:ogc),offset[k]+m,fh,lw,strlen(lw));
	}
	free(offset);
}

static int main_loop() {
	XEvent ev; XKeyEvent *e; KeySym key;
	int breakcode=0, tx=0, i, compcount=0, compcur=0, len=0, pos=0;
	int precomp = 0;
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
			if (key == 'a') {
				strcpy(line,complist[compcur]);
				pos = strlen(line);
			}
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
		else if (key == XK_Home) pos = 0;
		else if (key == XK_End) pos = strlen(line);
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
		else if (!iscntrl(*txt)) {
			part = strdup(&line[pos]);
			line[pos] = '\0';
			strncat(line,txt,len);
			strcat(line,part); free(part);
			pos+=len;
			compcheck = False;
		}
		if ( key == XK_Tab || key == XK_ISO_Left_Tab || 
			key == XK_Down || key == XK_Up ||
			( key != XK_Left && key != XK_Right &&
			breakcode == 0 && autocomp > 0 && pos >= autocomp ) ) {
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
				if (hush > -1) {
					comp = bangs[hush].comp;
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
				compcur = 0;
			}
		if ( compcheck && key == XK_Tab || key == XK_ISO_Left_Tab || 
			key == XK_Down || key == XK_Up) {
				if (compcur || pos == strlen(complist[compcur])) {
//					if ( (key==XK_Tab||key==XK_ISO_Left_Tab)&&compcount ) {
//						if (compcur == -1) compcur = 0;
//						strcpy(line,complist[compcur]);
//						pos = strlen(line);
//					}
					/*else*/ if ( key==XK_ISO_Left_Tab || key == XK_Up ) {
					if ((--compcur) < 0 ) compcur = compcount - 1;
					}
					else if ( (++compcur) >= compcount ) {
						compcur = 0;
					}
				}
				strcpy(line,complist[compcur]);
				pos = strlen(line);
			}
		}
		/* draw */
		XFillRectangle(dpy,buf,bgc,0,0,w,h);
		XmbDrawString(dpy,buf,xfs,gc,5,fh,line,strlen(line));
		tx = XmbTextEscapement(xfs,line,pos);
		XDrawLine(dpy,buf,gc,tx+5,2,tx+5,fh);
		if (show_opts && compcheck)
			options(compcount,(const char **)complist,compcur,tx);
		XCopyArea(dpy,buf,win,gc,0,0,w,h,0,0);
		XFlush(dpy);
		if (breakcode) break;
	}
	if (autocomp > 0 && compcheck && compcur != -1)
		strcpy(line,complist[compcur]);
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
		if (hush > -1) sprintf(cmd,bangs[hush].command,line);
		else strcpy(cmd,line);
	} 
	clean_up();
	if (strlen(cmd) > 2) {
		if (run_hook) {
			char *tmp = strdup(cmd);
			snprintf(cmd,MAX_LINE*2,run_hook,tmp);
			free(tmp); free(run_hook);
		}
		const char *argv[4]; argv[0] = "/bin/sh"; argv[1] = shell_flags;
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

