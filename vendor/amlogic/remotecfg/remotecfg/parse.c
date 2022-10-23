#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "common.h"

/*
remove head and tail blank in string
*/
static void StrTrim(char **s)
{
	int i;
	int len = strlen(*s);

	for (i = len - 1; i >= 0; i--) {
		if ((*s)[i] <= ' ')
			(*s)[i] = 0;
		else
			break;
	}

	while (**s) {
		if (**s <= ' ')
			(*s)++;
		else
			break;
	}
}

static void StrLineTrim(char *line, int length)
{
    char *tmp_ptr = line;

    if (line == NULL)
        return;
    /*trim comment string*/
    while (*tmp_ptr && ((tmp_ptr - line) <= length)) {
        if (*tmp_ptr == '#' || *tmp_ptr == ';') {
            *tmp_ptr = '\0';
            break;
        }
        tmp_ptr++;
    }
}

static int StrEmpty(char *str)
{
	int len = strlen(str);
	return (len == 0);
}

/*
Mapcode in file:
key_begin
	0x47 11
	0x13 2
	......
key_end
*/
static int GetMapCode(char *line, int *code)
{
	char *p2;
	char *p1 = line;
	int ircode;

	/*remove head and tail blank in string*/
	StrTrim(&line);

	/*get second data point in line*/
	p2 = strchr(line, ' ');
	if (p2) {
		*p2++ = 0;
		StrTrim(&p2);
	}
	if (!p2 || !*p2)
		return -1;
	StrTrim(&p1);
	if (!*p1)
		return -1;
	ircode = strtoul(p1, NULL, 0);
	if (ircode > 0xff) {
		fprintf(stderr, "invalid ircode: 0x%x\n", ircode);
		return -1;
	}

	*code = MAPCODE(ircode, strtoul(p2, NULL, 0));
	return 0;
}

int GetKeyValue(char *line, char **key, char **value)
{
	char *p1, *p2;

	p1 = line;
	p2 = strchr(line, '=');
	if (p2) {
		*p2++ = 0;
		StrTrim(&p2);
	}
	if (!p2 || !*p2)
		return -1;
	StrTrim(&p1);
	if (!p1 || !*p1)
		return -1;
	*value = p2;
	*key = p1;
	return 0;
}

static int ReadFile(FILE *fp, pfileHandle handler, void *data)
{
	char *key;
	char *value;
	unsigned int  mapcode;
	unsigned char parse_flag = CONFIG_LEVEL;
	char *line;
	char *savep;
	int ret;

	line = (char*)malloc(MAX_LINE_LEN + 1);
	if (!line) {
		fprintf(stderr, "malloc line failed\n");
		return -1;
	}
	savep = line;
	while (fgets(savep, MAX_LINE_LEN, fp)) {
		line = savep;
		StrLineTrim(line, MAX_LINE_LEN); /*jump comment*/
		if (StrEmpty(line))
			continue;
		StrTrim(&line);
		switch (parse_flag) {
		case CONFIG_LEVEL:
			if ((strcasecmp(line, "key_begin")) == 0) {
				parse_flag = KEYMAP_LEVEL;
				continue;
			}
			if (!!GetKeyValue(line, &key, &value))
				continue;
			if ((*handler)(key, value, data) < 0) {
				fprintf(stderr, "invalid line:%s=%s\n",
					line, value);
				continue;
			}
		break;
		case KEYMAP_LEVEL:
			if ((strcasecmp(line, "key_end")) == 0) {
				parse_flag = CONFIG_LEVEL;
				continue;
			}
			if (!!GetMapCode(line, &mapcode))
				continue;
			snprintf(line, 128, "%u", mapcode);
			if ((*handler)("mapcode", line, data) < 0) {
				fprintf(stderr, "invalid line:%s=%s\n",
					"mapcode", line);
				continue;
			}
		break;
		}

	}
	if (savep)
		free(savep);
	return 0;
}


int ParseFile(const char *file, pfileHandle handler, void *data)
{
	FILE *cfgFp;

	if (NULL == (cfgFp = fopen(file, "r"))) {
		fprintf(stderr, "fopen %s: %s\n", file, strerror(errno));
		return FAIL;
	}

	ReadFile(cfgFp, handler, data);
	fclose(cfgFp);
	return SUCC;
}
