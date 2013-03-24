
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#define MAX_LINE	122

typedef struct Bang {
	const char *bang;
	const char *command;
} Bang;

static Bang *bangs;
static int nbangs, scr, bh = 0, bw, fh;
static Display *dpy;
static Window root, win;
static Pixmap buf;
static GC gc, bgc;
static XFontStruct *fs;
static char bangchar = '!', colBG[8] = "#121212", colFG[8] = "#EEEEEE";
static char font[MAX_LINE] = "-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
static char line[MAX_LINE+4],bang[MAX_LINE],cmd[2*MAX_LINE];

static int config(int argc, const char **argv) {
	FILE *rc;
	char *c;
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '\0') rc = stdin;
	else { chdir(getenv("HOME")); rc = fopen(".interobangrc","r"); }
	if (!rc) return -1;
	while (fgets(line,MAX_LINE,rc) != NULL) {
		if (line[0] == '#' || line[0] == '\n') continue;
		else if (line[0] == bangchar && line[1] != '\0') {
			sscanf(line+1,"%s %[^\n]",bang,cmd);
			bangs = (Bang *) realloc(bangs,(nbangs + 1) * sizeof(Bang));
			bangs[nbangs].bang = strdup(bang);
			bangs[nbangs].command = strdup(cmd);
			nbangs++;
		}
		else if (strncmp(line,"background ",11)==0) {
			c = strchr(line,'#');
			if (c && strlen(c) > 6) strncpy(colBG,c,7);
		}
		else if (strncmp(line,"foreground ",11)==0) {
			c = strchr(line,'#');
			if (c && strlen(c) > 6) strncpy(colFG,c,7);
		}
		else if (strncmp(line,"font ",5)==0) {
			for (c = line + 4; *c == ' ' || *c == '\t'; c++);
			if (strlen(c) > 12) strncpy(font,c,strlen(c)-1);
		}
		else if (strncmp(line,"bangchar ",9)==0) {
			for (c = line + 8; *c == ' ' || *c == '\t'; c++);
			if (*c != '\n' && *c != '\0') bangchar = *c;
		}
		else {
			fprintf(stderr,"unrecognized configuration entry \"%s\"\n",line);
		}
	}
	if (rc != stdin) fclose(rc);
	strcpy(line,"");
	return 0;
}

static int init_X() {
	if (!(dpy=XOpenDisplay(0x0))) exit(1);
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	bw = DisplayWidth(dpy,scr);
	Colormap cmap = DefaultColormap(dpy,scr);
	XColor color;
	XGCValues val;
	fs = XLoadQueryFont(dpy,font);
	if (!fs) {fprintf(stderr,"unrecognized font\n"); exit(1);}
	val.font = fs->fid;
	XAllocNamedColor(dpy,cmap,colBG,&color,&color);
	val.foreground = color.pixel;
	bgc = XCreateGC(dpy,root,GCFont|GCForeground,&val);
	XAllocNamedColor(dpy,cmap,colFG,&color,&color);
	val.foreground = color.pixel;
	gc = XCreateGC(dpy,root,GCFont|GCForeground,&val);
	fh = fs->ascent + 1;
	if (!bh) bh = fh + fs->descent + 2;
	XSetWindowAttributes wa;
	wa.override_redirect = True;
	win = XCreateWindow(dpy,root,0,0,bw,bh,0,DefaultDepth(dpy,scr),
		InputOutput,DefaultVisual(dpy,scr),CWOverrideRedirect,&wa);
	buf = XCreatePixmap(dpy,root,bw,bh,DefaultDepth(dpy,scr));
	XMapWindow(dpy,win);
	XFillRectangle(dpy,buf,bgc,0,0,bw,bh);
	XCopyArea(dpy,buf,win,gc,0,0,bw,bh,0,0);
	XFlush(dpy);
	return 0;
}

static int main_loop() {
	XEvent ev;
	XKeyEvent *e;
	KeySym key;
	XGrabKeyboard(dpy,root,True,GrabModeSync,GrabModeAsync,CurrentTime);
	int breakcode = 0, x = 0, i, sp = 0;
	char *prefix = NULL;
	FILE *compgen; Bool compcheck = False;
	char **complist = NULL; int compcount = 0, compcur = 0;
	while (!XNextEvent(dpy,&ev)) {
		if (ev.type != KeyPress) continue;
		/* get key */
		e = &ev.xkey;
		key = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
		if (key == XK_Return) breakcode = 1;
		else if (key == XK_Escape) breakcode = -1;
		else if (key == XK_Tab) {
			if (!compcheck) {
				if (complist) {
					for (i = 0; i < compcount; i++) free(complist[i]);
					free(complist);
					complist = NULL;
					compcount = 0;
				}
				if (sp < strlen(line)) { prefix = strdup(line); prefix[sp] = '\0'; }
				if (prefix)
					sprintf(cmd,"compgen -P \"%s\" -cf %s ",prefix,line + sp);
				else
					sprintf(cmd,"compgen -cf %s",line);
				if (prefix) { free(prefix); prefix = NULL; }
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
				if ( (++compcur) >= compcount ) compcur = 0;
				strcpy(line,complist[compcur]);
			}
		}
		else if (key == XK_BackSpace) {
			line[strlen(line) - 1] = '\0';
			compcheck = False;
		}
		else if (key == XK_Delete) {
			line[0] = '\0';
			compcheck = False;
		}
		else if (key == XK_space) {
			strcat(line," ");
			sp = strlen(line);
		}
		else {
			char buf[10];
			int len = XLookupString(e,buf,9,NULL,NULL);
			strncat(line,buf,len);
			compcheck = False;
		}
		/* draw */
		XFillRectangle(dpy,buf,bgc,0,0,bw,bh);
		XDrawString(dpy,buf,gc,5,fh,line,strlen(line));
		x = XTextWidth(fs,line,strlen(line)) + 6;
		XDrawLine(dpy,buf,gc,x,2,x,fh);
		XCopyArea(dpy,buf,win,gc,0,0,bw,bh,0,0);
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
	int i, x; char *c;
	strcpy(cmd,"");
	if (line[0] == bangchar) { /* "bang" syntax: */
		c = strchr(line,' ');
		x = c - line - 1; /* length of bang */
		if (line[1] == ' ' && nbangs != 0)
			sprintf(cmd,"%s%s &",bangs[0].command, c);
		else for (i = 0; i < nbangs; i++)
			if (strncmp(bangs[i].bang,line + 1,x) == 0)
				sprintf(cmd,"%s%s &",bangs[i].command, c);
	}
	else {
		strcpy(cmd,line);
		strcat(cmd," &");
	}
	if (strlen(cmd) > 2) system(cmd);
}

int main(int argc, const char **argv) {	
	config(argc,argv);
	init_X();
	if (main_loop())
		process_command();
	XFreeFont(dpy,fs);
	XDestroyWindow(dpy,win);
	XCloseDisplay(dpy);
	return 0;
}

