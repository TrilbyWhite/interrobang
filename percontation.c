
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_LINE	256

#define OPT_NORM	0x00
#define OPT_EXEC	0x01
#define OPT_DIR	0x02
#define OPT_PATH	0x04
#define OPT_DESK	0x10
#define OPT_DCOM	0x20
#define OPT_DEXE	0x40

static const char *pre, *str;
static const char deskchar = '>';


static int get_name(const char *path,int opt) {
	DIR *dir;
	FILE *f;
	struct dirent *de;
	struct stat info;
	int opts, match, term=0;
	char line[MAX_LINE], *name=NULL, *c, *c2, *exec=NULL;
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
		if (opt & OPT_DEXE) {
			name = strdup(de->d_name);
			if ( (c=strrchr(name,'.')) ) *c='\0';
			if (strncmp(name,str,strlen(name))) continue;
			if ( !(str[strlen(name)] == ' ' || str[strlen(name)] == '\0') )
				continue;
			f = fopen(de->d_name,"r");
			while(fgets(line,MAX_LINE,f)) {
				if (strncmp(line,"Exec=",5)==0) exec=strdup(line+5);
				if (strncmp(line,"Terminal=true",13)==0) term=1;
			}
			fclose(f);
			if (name) { free(name); name=NULL; }
			if (!exec) return 1;
			if ( (c2=strstr(exec,"%u")) ) {	
				*(++c2) = 's';
				if ( (c=strchr(str,' ')) ) {
					c++;
					name = calloc(strlen(exec)+strlen(c)+2,sizeof(char));
					sprintf(name,exec,c);
				}
				else {
					name = calloc(strlen(exec)+2,sizeof(char));
					sprintf(name,exec,"");
				}
				free(exec); exec=strdup(name); free(name); name=NULL;
			}
			if (term) printf("!term %s",exec);
			else printf("%s",exec);
			if (exec) {free(exec); exec=NULL;}
		}
		else if (opt & OPT_DESK) {
			name = strdup(de->d_name);
			if ( (c=strrchr(name,'.')) ) *c='\0';
			if (!(opt & OPT_DCOM)) { /* desktop */
				if (!strncmp(de->d_name,str,strlen(str)))
					printf("%c%s\n",deskchar,name);
			}
			else { /* desktop+ */
				f = fopen(de->d_name,"r");
				match = 0;
				while(fgets(line,MAX_LINE,f)) {
//prin	tf(":: (%s) %s",str,line);
					if (	(strncmp(line,"Name",4)==0 ||
							strncmp(line,"GenericName",11)==0) &&
							(c=strchr(line,'=')) && c++ &&
							strncasecmp(str,c,strlen(str))==0 )
						match = 1;
					if (	strncmp(line,"Comment",7)==0 &&
							(c=strchr(line,'=')) && c++ &&
							strcasestr(c,str) )
						match=1;
				}
				if (name && match) printf("%c%s\n",deskchar,name);
				fclose(f);
			}
			if (name) { free(name); name=NULL; }
		}
		else {
			if (strncmp(de->d_name,str,strlen(str))) continue;
			stat(de->d_name,&info);
			if( (opt & OPT_EXEC) && info.st_mode & 0111) opts = opt;
			else if (S_ISDIR(info.st_mode)) opts = opt | OPT_DIR;
			else opts = opt;
			printf(pre);
			if (opts & OPT_PATH) printf("%s/",path);
			if (opts & OPT_DIR) printf("%s/\n",de->d_name);
			else printf("%s\n",de->d_name);
		}
	}
	closedir(dir);
	return 0;
}	

static int from_path() {
	char *path;
	const char *pwd = getenv("PWD");
	if ( !(path=getenv("PATH")) ) return 2;
	char *list = strdup(path), *next = list, *delim;
	while ( (delim=strchr(next,':')) ) {
		*delim = '\0';
		get_name(next,OPT_EXEC);
		next = delim+1;
	}
	if (str[0] == '/') {
		path = strdup(str);
		char *c = strrchr(path,'/');
		*c = '\0';
		str = strrchr(str,'/');
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

static int from_desktops(int comments) {
	// only run this if pre is empty
	// for all paths to .desktop files
	get_name("/usr/share/applications",
			OPT_DESK | (comments ? OPT_DCOM : 0));
	return 0;
}

static int desk_exec() {
	str++;
	get_name("/usr/share/applications",OPT_DEXE);
	return 0;
}

int main(int argc, const char **argv) {
	int i;
	if (argc < 3) return 1;
	pre = argv[argc - 2];
	str = argv[argc - 1];
	for (i = 1; i < argc - 2; i++) {
		if (strncmp(argv[i],"path",4)==0) from_path();
		else if (strncmp(argv[i],"desktop+",9)==0) from_desktops(1);
		else if (strncmp(argv[i],"desktop",8)==0) from_desktops(0);
		else if (strncmp(argv[i],"deskexec",9)==0) desk_exec();
	}
	if (i == 1) from_path();
	return 0;
}


