
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#define MAX_LINE 256
#define TERM_CMD "urxvt -e "

static char *desktop_name = NULL;
static int term = 0;
static const char *paths[] = {
	"/usr/share/applications",
	"~/.local/share/applications",
	NULL
};
	

static FILE *get_desktop(const char *path, const char *name) {
	FILE *f = NULL;
	DIR *dir;
	struct dirent *de;
	dir=opendir(path);
	if (!dir) return NULL;
	char *fname = calloc(strlen(name)+10,sizeof(char));
	strcpy(fname,name); strcat(fname,".desktop");
	chdir(path);
	while ( (de=readdir(dir)) ) {
		if (de->d_name[0] == '.') continue;
		if (!strncmp(de->d_name,fname,strlen(fname)))
			f = fopen(fname,"r");
		if (f) break;
	}
	desktop_name = realloc(desktop_name,(strlen(fname)+2)*sizeof(char));
	strcpy(desktop_name,fname); strcat(desktop_name," ");
	free(fname);
	return f;
}

static int parse_command(const char *cmd, const char *args) {
	int i,j = 0, len = strlen(cmd) + strlen(args);
	if (strstr(cmd,"%k")) len += strlen(desktop_name);
	char c, *t = NULL, *full = calloc(len,sizeof(char));
	if (term) { strcpy(full,TERM_CMD); j=strlen(TERM_CMD) + 1; }
	strcat(full,cmd);
	for (i = 0; i < strlen(cmd); i++, j++) {
		if (cmd[i] != '%') continue;
		c = cmd[(++i)]; i++;
		if (c == 'f' || c == 'F' || c == 'u' || c == 'U') t = (char *) args;
		else if (c == 'k') t = desktop_name;
		else if (c == '%') full[j++] = '%';
		else continue;
		strcpy(full+j,t);
		j += strlen(t);
	}
	if (fork() == 0) {
		char *args[4];
		args[0] = "/usr/bin/bash"; args[1] = "-c";
		args[2] = full; args[3] = NULL;
		execvp(args[0],(char * const *) args);
	}
	free(full);
	return 0;
	}

int main(int argc, const char **argv) {
	if (argc < 2) return 1;
	int i;
	// For each path:
	FILE *desk=NULL, *tdesk;
	for (i = 0; paths[i]; i++) {
		char *fullpath;
		if (paths[i][0] == '~') {
			char *home = getenv("HOME");
			fullpath = calloc(strlen(paths[i])+strlen(home),sizeof(char));
			strcpy(fullpath,home); strcat(fullpath,paths[i]+1);
		}
		else {
			fullpath = strdup(paths[i]);
		}
		tdesk=get_desktop(fullpath,argv[1]);
		free(fullpath);
		if (tdesk) {
			if (desk) fclose(desk);
			desk=tdesk;
		}
	}
	if (!desk) return 2;
	char line[MAX_LINE], *cmd = NULL;
	while (fgets(line,MAX_LINE,desk)) {
		if ( !strncmp(line,"Exec=",5) ) cmd = strdup(line+5);
		if ( !strncmp(line,"Terminal=true",12) ) term=1;
	}
	fclose(desk);
	if (strstr(cmd,"%f")) {
		for (i = 2; i < argc; i++)
			parse_command(cmd,argv[i]);
	}
	else {
		int len = 0;
		for (i = 2; i < argc; i++) len += strlen(argv[i]) + 1;
		char *args = calloc(len,sizeof(char));
		for (i = 2; i < argc; i++) {
			strcat(args,argv[i]);
			strcat(args," ");
		}
		parse_command(cmd,args);
		free(args);
	}
	free(cmd);
	free(desktop_name);
	return 0;
}
