#if !defined(ED_INIFILE_C)
#define ED_INIFILE_C

#if defined(__gnu_linux__)
#define _GNU_SOURCE 1
#endif

#if !defined(_MSC_VER)
#define _strdup strdup
#endif

#include <string.h>
#include "ED_locale.h"
#include "array.h"
#define INI_BUFFERSIZE 1024
#include "minIni.h"
#include "ModelicaUtilities.h"
#include "../Include/ED_INIFile.h"

typedef struct {
	char* key;
	char* value;
} INIPair;

typedef struct {
	char* name;
	cpo_array_t* pairs;
} INISection;

typedef struct {
	char* fileName;
	ED_LOCALE_TYPE loc;
	cpo_array_t* sections;
} INIFile;

static int compareSection(const void *a, const void *b)
{
    return strcmp(((INISection*)a)->name, ((INISection*)b)->name);
}

static int compareKey(const void *a, const void *b)
{
    return strcmp(((INIPair*)a)->key, ((INIPair*)b)->key);
}

static INISection* findSection(INIFile* ini, const char* name)
{
    INISection tmpSection = {(char*)name, NULL};
    INISection* ret = (INISection*)cpo_array_bsearch(ini->sections, &tmpSection, compareSection);
    return ret;
}

static INIPair* findKey(INISection* section, const char* key)
{
    INIPair tmpPair = {(char*)key, NULL};
    INIPair* ret = (INIPair*)cpo_array_bsearch(section->pairs, &tmpPair, compareKey);
    return ret;
}

/* Callback function for ini_browse */
static int fillValues(const char *section, const char *key, const char *value, const void *userdata)
{
	INIFile* ini = (INIFile*)userdata;
	if (ini != NULL) {
		INIPair* pair;
		INISection* _section = findSection(ini, section);
		if (_section == NULL) {
			_section = (INISection*)cpo_array_push(ini->sections);
			_section->name = (section != NULL) ? _strdup(section) : NULL;
			_section->pairs = cpo_array_create(4 , sizeof(INIPair));
		}
		pair = (INIPair*)cpo_array_push(_section->pairs);
		pair->key = (key != NULL) ? _strdup(key) : NULL;
		pair->value = (value != NULL) ? _strdup(value) : NULL;
		return 1;
	}
	return 0;
}

void* ED_createINI(const char* fileName) {
	INIFile* ini = (INIFile*)malloc(sizeof(INIFile));
	if (ini != NULL) {
		ini->fileName = _strdup(fileName);
		if (ini->fileName == NULL) {
			free(ini);
			ModelicaError("Memory allocation error\n");
			return NULL;
		}
		ini->sections = cpo_array_create(1 , sizeof(INISection));
		if (1 != ini_browse(fillValues, ini, fileName)) {
			ModelicaFormatError("Error: Cannot read \"%s\"\n", fileName);
			return NULL;
		}
		ini->loc = ED_INIT_LOCALE;
	}
	else {
		ModelicaError("Memory allocation error\n");
		return NULL;
	}
	return ini;
}

void ED_destroyINI(void* _ini)
{
	INIFile* ini = (INIFile*)_ini;
	if (ini != NULL) {
		if (ini->fileName != NULL) {
			free(ini->fileName);
		}
		ED_FREE_LOCALE(ini->loc);
		if (ini->sections != NULL) {
			int i;
			for (i = 0; i < ini->sections->num; i++) {
				INISection* section = (INISection*)cpo_array_get_at(ini->sections, i);
				free(section->name);
				if (section->pairs != NULL) {
					int j;
					for (j = 0; j < section->pairs->num; j++) {
						INIPair* pair = (INIPair*)cpo_array_get_at(section->pairs, j);
						free(pair->key);
						free(pair->value);
					}
					cpo_array_destroy(section->pairs);
				}
			}
			cpo_array_destroy(ini->sections);
		}
		free(ini);
	}
}

double ED_getDoubleFromINI(void* _ini, const char* varName, const char* section)
{
	double ret = 0.;
	INIFile* ini = (INIFile*)_ini;
	if (ini != NULL) {
		INISection* _section = findSection(ini, section);
		if (_section != NULL) {
			INIPair* pair = findKey(_section, varName);
			if (pair != NULL) {
				if (ED_strtod(pair->value, ini->loc, &ret)) {
					ModelicaFormatError("Error when reading double value \"%s\" from file \"%s\"\n",
						pair->value, ini->fileName);
				}
			}
			else {
				ModelicaFormatError("Error when reading key \"%s\" from file \"%s\"\n",
					varName, ini->fileName);
			}
		}
		else {
			if (strlen(section) > 0) {
				ModelicaFormatError("Error when reading section \"%s\" from file \"%s\"\n",
					section, ini->fileName);
			}
			else {
				ModelicaFormatError("Error when reading empty section from file \"%s\"\n",
					ini->fileName);
			}
		}
	}
	return ret;
}

const char* ED_getStringFromINI(void* _ini, const char* varName, const char* section)
{
	INIFile* ini = (INIFile*)_ini;
	if (ini != NULL) {
		INISection* _section = findSection(ini, section);
		if (_section != NULL) {
			INIPair* pair = findKey(_section, varName);
			if (pair != NULL) {
				char* ret = ModelicaAllocateString(strlen(pair->value));
				strcpy(ret, pair->value);
				return (const char*)ret;
			}
			else {
				ModelicaFormatError("Error when reading key \"%s\" from file \"%s\"\n",
					varName, ini->fileName);
			}
		}
		else {
			if (strlen(section) > 0) {
				ModelicaFormatError("Error when reading section \"%s\" from file \"%s\"\n",
					section, ini->fileName);
			}
			else {
				ModelicaFormatError("Error when reading empty section from file \"%s\"\n",
					ini->fileName);
			}
		}
	}
	return "";
}

int ED_getIntFromINI(void* _ini, const char* varName, const char* section)
{
	int ret = 0;
	INIFile* ini = (INIFile*)_ini;
	if (ini != NULL) {
		INISection* _section = findSection(ini, section);
		if (_section != NULL) {
			INIPair* pair = findKey(_section, varName);
			if (pair != NULL) {
				if (ED_strtoi(pair->value, ini->loc, &ret)) {
					ModelicaFormatError("Error when reading int value \"%s\" from file \"%s\"\n",
						pair->value, ini->fileName);
				}
			}
			else {
				ModelicaFormatError("Error when reading key \"%s\" from file \"%s\"\n",
					varName, ini->fileName);
			}
		}
		else {
			if (strlen(section) > 0) {
				ModelicaFormatError("Error when reading section \"%s\" from file \"%s\"\n",
					section, ini->fileName);
			}
			else {
				ModelicaFormatError("Error when reading empty section from file \"%s\"\n",
					ini->fileName);
			}
		}
	}
	return ret;
}

#endif
