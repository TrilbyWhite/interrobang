
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

#define TERM	"urxvtc -e"

static const Bang bangs[] = {
	/* an empty bang defaults to the first entry: */
	{ "term",				TERM	},
	{ "web",				"luakit"	},
	{ "pdf",				"mupdf"		},
	{ "man",				TERM " man" },
};

int main(int argc, const char **argv) {	
/* DEFAULTS: */
	const char *font = "-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
	const char *colBG = "#121212";
	const char *colFG = "#EEEEEE";
/* INIT X */
	Display *dpy; if (!(dpy=XOpenDisplay(0x0))) return 1;
	int scr = DefaultScreen(dpy);
	Window win, root = RootWindow(dpy,scr);
	Pixmap buf;
	int bh = 0, bw = DisplayWidth(dpy,scr);
	Colormap cmap = DefaultColormap(dpy,scr);
	XColor color;
	XGCValues val;
	XFontStruct *fs = XLoadQueryFont(dpy,font);
	val.font = fs->fid;
	XAllocNamedColor(dpy,cmap,colBG,&color,&color);
	val.foreground = color.pixel;
	GC bgc = XCreateGC(dpy,root,GCFont|GCForeground,&val);
	XAllocNamedColor(dpy,cmap,colFG,&color,&color);
	val.foreground = color.pixel;
	GC gc = XCreateGC(dpy,root,GCFont|GCForeground,&val);
	int fh = fs->ascent + 1;
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
/* MAIN LOOP */
	XEvent ev;
	XKeyEvent *e;
	KeySym key;
	XGrabKeyboard(dpy,root,True,GrabModeSync,GrabModeAsync,CurrentTime);
	int run = 1, x = 0, i, sp = 0;
	char line[MAX_LINE + 4] = "", cmd[MAX_LINE + 24] = "", *prefix = NULL;
	FILE *compgen; Bool compcheck = False;
	char **complist = NULL; int compcount = 0, compcur = 0;
	while (!XNextEvent(dpy,&ev)) {
		if (ev.type != KeyPress) continue;
		/* get key */
		e = &ev.xkey;
		key = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
		if (key == XK_Return) run = 0;
		else if (key == XK_Escape) run = -1;
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
		if (run != 1) break;
	}
	if (complist) {
		for (i = 0; i < compcount; i++) free(complist[i]);
		free(complist);
		complist = NULL;
		compcount = 0;
	}
	XUngrabKeyboard(dpy,CurrentTime);
/* PROCESS COMMAND */
	if (run == 0) {
		strcpy(cmd,"");
		if (line[0] == '!') { /* tmenu "bang" syntax: */
			prefix = strchr(line,' ');
			x = prefix - line - 1; /* length of bang */
			if (line[1] == ' ')
				sprintf(cmd,"%s%s &",bangs[0].command, prefix);
			else for (i = 0; i < sizeof(bangs)/sizeof(bangs[0]); i++)
				if (strncmp(bangs[i].bang,line + 1,x) == 0)
					sprintf(cmd,"%s%s &",bangs[i].command, prefix);
		}
		else {
			strcpy(cmd,line);
			strcat(cmd," &");
		}
		if (strlen(cmd) > 2) system(cmd);
	}
/* CLEAN UP */
	XFreeFont(dpy,fs);
	XDestroyWindow(dpy,win);
	XCloseDisplay(dpy);
	return 0;
}

