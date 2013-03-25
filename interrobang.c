
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#define MAX_LINE	122

typedef struct Bang {
	char *bang;
	char *command;
} Bang;

static Bang *bangs;
static int nbangs, scr, fh, x = 0, y = 0, w = 0, h = 0;
static Display *dpy;
static Window root, win;
static Pixmap buf;
static GC gc, bgc;
static XFontStruct *fs;
static char bangchar = '!', colBG[8] = "#121212", colFG[8] = "#EEEEEE", colBD[8] = "";
static char font[MAX_LINE] = "-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
static char line[MAX_LINE+4],bang[MAX_LINE],cmd[2*MAX_LINE];

static int config(int argc, const char **argv) {
	FILE *rc;
	char *c;
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '\0') rc = stdin;
	else { chdir(getenv("HOME")); rc = fopen(".interrobangrc","r"); }
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
		else if (strncmp(line,"border ",7)==0) {
			c = strchr(line,'#');
			if (c && strlen(c) > 6) strncpy(colBD,c,7);
		}
		else if (strncmp(line,"font ",5)==0) {
			for (c = line + 4; *c == ' ' || *c == '\t'; c++);
			if (strlen(c) > 12) strncpy(font,c,strlen(c)-1);
		}
		else if (strncmp(line,"bangchar ",9)==0) {
			for (c = line + 8; *c == ' ' || *c == '\t'; c++);
			if (*c != '\n' && *c != '\0') bangchar = *c;
		}
		else if (strncmp(line,"geometry ",9)==0) {
			for (c = line + 8; *c == ' ' || *c == '\t'; c++);
			if (*c != '\n' && *c != '\0') {
				if (*c == 'b') y = -1;
				else sscanf(c,"%dx%d+%d+%d",&w,&h,&x,&y);
			}
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
	w = (w ? w : DisplayWidth(dpy,scr) - (strlen(colBD)?2:0));
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
	if (!h) h = fh + fs->descent + 2;
	if (y == -1) y = DisplayHeight(dpy,scr) - h;
	XSetWindowAttributes wa;
	wa.override_redirect = True;
	wa.border_pixel = (XAllocNamedColor(dpy,cmap,colBD,&color,&color) ?
		color.pixel : 0);
	win = XCreateWindow(dpy,root,x,y,w,h,(strlen(colBD)?1:0),
		DefaultDepth(dpy,scr),InputOutput,DefaultVisual(dpy,scr),
		CWOverrideRedirect|CWBorderPixel,&wa);
	buf = XCreatePixmap(dpy,root,w,h,DefaultDepth(dpy,scr));
	XMapWindow(dpy,win);
	XFillRectangle(dpy,buf,bgc,0,0,w,h);
	XCopyArea(dpy,buf,win,gc,0,0,w,h,0,0);
	XFlush(dpy);
	return 0;
}

static int main_loop() {
	XEvent ev;
	XKeyEvent *e;
	KeySym key;
	int breakcode = 0, x = 0, i;
	for (i = 0; i < 1000; i++) {
		if (XGrabKeyboard(dpy,root,True,GrabModeAsync,GrabModeAsync,
			CurrentTime) == GrabSuccess) break;
		usleep(1000);
	}
	if (i == 1000) exit(1);
	unsigned int mod;
	char prefix[MAX_LINE+3], *sp = NULL;
	FILE *compgen; Bool compcheck = False;
	char **complist = NULL; int compcount = 0, compcur = 0;
	while (!XNextEvent(dpy,&ev)) {
		if (ev.type != KeyPress) continue;
		/* get key */
		e = &ev.xkey;
		key = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
		mod = ((e->state&~Mod2Mask)&~LockMask);
		if (mod & ControlMask) {
			if (key == 'u') line[0] = '\0';
		}
		else if (key == XK_Return) breakcode = 1;
		else if (key == XK_Escape) breakcode = -1;
		else if (key == XK_BackSpace) line[strlen(line) - 1] = '\0';
		else if (key == XK_Delete) line[0] = '\0';
		else if (key == XK_space) strcat(line," ");
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
				sprintf(cmd,"compgen -P \"%s\" -cf %s",prefix,sp);
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
				strcat(line," ");
			}
		}
		else {
			char buf[10];
			int len = XLookupString(e,buf,9,NULL,NULL);
			strncat(line,buf,len);
			compcheck = False;
		}
		if (key != XK_Tab) compcheck = False;
		/* draw */
		XFillRectangle(dpy,buf,bgc,0,0,w,h);
		XDrawString(dpy,buf,gc,5,fh,line,strlen(line));
		x = XTextWidth(fs,line,strlen(line)) + 6;
		XDrawLine(dpy,buf,gc,x,2,x,fh);
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
		/* x = length of bang */
		if ( (c=strchr(line,' ')) != NULL) x = c - line - 1;
		else x = strlen(line + 1);
		/* b = bang command */
		if (x) for (i = 0; i < nbangs; i++)
			if (strncmp(bangs[i].bang,line + 1,x) == 0)
				b = bangs[i].command;
		if (!b && nbangs != 0) b = bangs[0].command;
		if (b && c) { c++; sprintf(cmd,b,c); }
		else if (b)	sprintf(cmd,b, line + 1);
	}
	else {
		strcpy(cmd,line);
	} 
	strcat(cmd," &");
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

