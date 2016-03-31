
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static const int MAX_LINE = 256;
static const char deskchar = '>';

static int get_name(const char *pre, const char *path, const char *match, int mode) {
	DIR *dir;
	struct dirent *de;
	struct stat info;
	const char *p = (path[0] == '\0' ? "/" : path);
	if (!(dir=opendir(p))) return 1;
	chdir(p);
	while ( (de=readdir(dir)) ) {
		if (de->d_name[0] == '.' || strncmp(de->d_name, match, strlen(match))) continue;
		stat(de->d_name, &info);
		if(!(info.st_mode & mode)) continue;
		printf(pre);
		if (mode & 0100000 || pre[0] != '\0') printf("%s/",path);
		if (S_ISDIR(info.st_mode)) printf("%s/\n", de->d_name);
		else printf("%s\n", de->d_name);
	}
	closedir(dir);
	return 0;
}

static int from_path(const char *pre, const char *str) {
	char *path, *next;
	const char *name;
	int mode = 0;
	if (str[0] == '/') {
		path = strdup(str);
		*strrchr(path,'/') = '\0';
		name = strrchr(str,'/') + 1;
		mode = 01000000;
	}
	else {
		path = strdup(getenv("PWD"));
		name = str;
	}
	if (!pre || pre[0] == '\0') {
		if ( !(path=strdup(getenv("PATH"))) ) return 1;
		for (next = strtok(path,":"); next; next = strtok(NULL,":")) get_name(pre, next, name, mode | 0111);
	}
	else {
		get_name(pre, path, name, mode | 0444);
	}
	free(path);
	return 0;
}

static int from_desktop(int in, const char *str) {
	DIR *dir;
	FILE *fp;
	char *name, *c, line[MAX_LINE];
	if ( !(dir=opendir("/usr/share/applications")) ) return 1;
	chdir("/usr/share/applications");
	struct dirent *de;
	while ( (de=readdir(dir)) ) {
		if (de->d_name[0] == '.') continue;
		name = strdup(de->d_name);
		if ( (c=strrchr(name,'.')) ) *c = '\0';
		if (!strncmp(de->d_name, str, strlen(str))) printf("%c%s\n",deskchar,name);
		else if (in) {
			if (!(fp=fopen(de->d_name, "r"))) continue;
			while (fgets(line,MAX_LINE,fp)) {
				if (strncmp(line,"Name=",5) && strncmp(line,"GenericName=",12)) continue;
				if (!(c=strchr(line,'='))) continue;
				if (!strncasecmp(++c,str,strlen(str))) printf("%c%s\n",deskchar,name);
			}
			fclose(fp);
		}
		free(name);
	}
	return 0;
}

int main(int argc, const char **argv) {
	int i;
	if (argc < 3) return 1;
	for (i = 1; i < argc - 2; i++) {
		if (!strncmp(argv[i],"path",4)) from_path(argv[argc-2],argv[argc-1]);
		else if (!strncmp(argv[i],"desktop+",9)) from_desktop(1,argv[argc-1]);
		else if (!strncmp(argv[i],"desktop",8)) from_desktop(0,argv[argc-1]);
	}
	if (i == 1) from_path(argv[argc-2],argv[argc-1]);
	return 0;
}


