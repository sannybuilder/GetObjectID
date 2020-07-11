#include <stdio.h>

extern __declspec(dllimport) int GetObjectID (char * objname, char * path);
extern __declspec(dllexport) int GetObjectName (int id, char * path, char * objname);

void main (int argc, char ** argv) {
	int id;

 	if (argc < 3) {
		printf ("USAGE: %s game_dir object_name_or_id\n", argv[0]);
		return;
	}
	if (isdigit (*argv[2])) {	// name by id
		char name[32];
		if (GetObjectName (atoi (argv[2]), argv[1], name)) 
			printf ("object #%s name='%s'\n", argv[2], name);
		else
			printf ("object #%s not found\n", argv[2]);
	} else {
		int id = GetObjectID (argv[2], argv[1]);
		if (id < 0) 
			printf ("object '%s' not found\n", argv[2]);
		else 
			printf ("object '%s' id=%d\n", argv[2], id);			
	}
}
