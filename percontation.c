
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define OPT_NORM	0x00
#define	OPT_EXEC	0x01
#define OPT_DIR		0x02
#define OPT_PATH	0x04

static const char *pre, *str;

static int add(const char *path, const char *name,int opt) {
	printf(pre);
	if (opt & OPT_PATH) printf("%s/",path);
	if (opt & OPT_DIR) printf("%s/\n",name);
	else printf("%s\n",name);
}

static int get_name(const char *path,int opt) {
	DIR *dir;
	struct dirent *de;
	struct stat info;
	if (path[0] == '\0') {
		dir = opendir("/");
		chdir("/");
	}
	else {
		dir = opendir(path);
		chdir(path);
	}
	if (!dir) return 1;
	while ( (de=readdir(dir)) ) {
		if (de->d_name[0] == '.') continue;
		if (strncmp(de->d_name,str,strlen(str))) continue;
		stat(de->d_name,&info);
		if( (opt & OPT_EXEC) && info.st_mode & 0111) add(path,de->d_name,opt);
		else if (S_ISDIR(info.st_mode)) add(path,de->d_name,opt | OPT_DIR);
		else add(path,de->d_name,opt);
	}
	closedir(dir);
	return 0;
}	

int main(int argc, const char **argv) {
	char *path;
	const char *pwd = getenv("PWD");
	if ( !((argc==3) && (path=getenv("PATH"))) ) return 1;
	pre = argv[1];
	str = argv[2];
	char *list = strdup(path), *next = list, *delim;
	while ( (delim=strchr(next,':')) ) {
		*delim = '\0';
		get_name(next,OPT_EXEC);
		next = delim+1;
	}
	if (str[0] == '/') {
		path = strdup(argv[2]);
		char *c = strrchr(path,'/');
		*c = '\0';
		str = strrchr(argv[2],'/');
		str++;
		if (path[0] == '\0') get_name("",OPT_PATH);
		else get_name(path,OPT_PATH);
		free(path);
	}
	else {
		get_name(pwd,OPT_NORM);
	}
	chdir(pwd);
	return 0;
}
