#include <stdio.h>
#include <ctype.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>

char lineBuffer[512];

// коллбэк для поиска. 
// возвращает ненулевое значение, если значение найдено. 
// поиск производится в lineBuffer.
typedef int (*searchCB)(char * objName, int * id);

int searchIdByName (char * objname, int * id) {
	char * p = lineBuffer;
	if (!isdigit (*lineBuffer))
		return 0;
	while (isdigit (*p))
		p++;		
	while (isspace (*p) || *p == ',') {
		if (!*p)
			return 0;
		p++;
	}
	while (*objname) {
	 	if (*objname++ != toupper(*p++))
			return 0;
		if (!*p)
			return 0;
	}
	if (isspace (*p) || *p == ',') {
		*id = atoi (lineBuffer);
		return 1;
	}
	return 0;

}

int searchNameById (char * objname, int * id) {
	int pos; // для проверки длины буфера
	char * p = lineBuffer;

	if (!isdigit (*lineBuffer))
		return 0;

	while (isdigit (*p))
		p++;		
	*p++ = '\0';

	if (atoi (lineBuffer) != *id)
		return 0;

	while (isspace (*p) || *p == ',') {
		if (!*p)
			return 0;
		p++;
	}
	for (pos = 0; *p && pos < 31 && !isspace(*p) && *p != ','; p++, objname++, pos++) 
		*objname = *p;
	*objname = '\0';
	return 1;
}

int parseIde (char * filename, char * objname, int * id, searchCB cb) {
	int found = 0;
	FILE * f = fopen (filename, "r");

	if (!f) 
		return -1;	
	while (fgets (lineBuffer, 512, f)) 
		if ((found = cb (objname, id)))
			break;

	fclose (f);
	return found;
}

int traversePath (char * path, char * objname, int * id, searchCB cb) {
	struct _finddata_t find;
	intptr_t file;
	char pathBuffer[_MAX_PATH];
	char * p;
	int found = 0;

	sprintf (pathBuffer, "%s/*.*", path);
	if ((file = _findfirst (pathBuffer, &find)) == -1)
		return -1;

	do {	
		char * ext;

		if (find.name[0] == '.' && ((find.name[1] == '.' && find.name[2] == '\0') || find.name[1] == '\0'))
			continue; // skip "." and ".."

		if (find.attrib & _A_SUBDIR) {
			sprintf (pathBuffer, "%s/%s", path, find.name);
			// printf ("D: '%s'\n", pathBuffer);
			if ((found = traversePath (pathBuffer, objname, id, cb)))
				break;
			continue;
		}

		ext = strrchr (find.name, '.');
		if (ext && !stricmp (ext, ".ide")) {
			sprintf (pathBuffer, "%s/%s", path, find.name);
			// printf (">> '%s'\n", pathBuffer);
			if ((found = parseIde (pathBuffer, objname, id, cb)))
				break;
		}
	} while (!_findnext (file, &find));
	_findclose (file);
	return found;
}

__declspec(dllexport) int GetObjectID (char * objname, char * path) {
	int id = -1;
	char * p;
	for (p = objname; *p; p++)
		*p = toupper (*p);
	return traversePath (path, objname, &id, searchIdByName) ? id : -1;
}

__declspec(dllexport) int GetObjectName (int id, char * path, char * objname) {
	return traversePath (path, objname, &id, searchNameById);
}