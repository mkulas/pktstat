#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "abbrev.h"

struct pattern {
	const char *name;
	const char *pattern;
};

static int npatterns;
static int allocpatterns;
static struct pattern *patterns;

static int abbrev_match(const char *, const char *);

static int
abbrev_match(tag, pattern)
	const char *tag, *pattern;
{
	while (*pattern == '*') {
		while (*tag) {
			if (abbrev_match(tag+1, pattern+1))
				return 1;
			tag++;
		}
		pattern++;
	}
	if (*pattern != *tag)
		return 0;
	else if (*tag == '\0')	/* and *pattern==0 */
		return 1;
	else
		return abbrev_match(tag+1, pattern+1);
}

/*
 * Search through the abbreviation patterns, returning the first pattern
 * that matches the tag; otherwise return the original tag.
 */
const char *
abbrev_tag(tag)
	const char *tag;
{
	int i;

	if (tag)
		for (i = 0; i < npatterns; i++)
			if (abbrev_match(tag, patterns[i].pattern))
				return patterns[i].name;
	return tag;
}

/* Add an abbreviation pattern to the list of known patterns */
void
abbrev_add(abbrev)
	const char *abbrev;
{
	char *name, *pattern, *p;

	while (npatterns >= allocpatterns) {
		if (allocpatterns > 0)
			patterns = (struct pattern *)realloc(patterns, 
			   sizeof *patterns * (allocpatterns *= 2));
		else
			patterns = (struct pattern *)malloc(sizeof *patterns *
				(allocpatterns = 16));
		if (patterns == NULL)
			errx(1, "malloc/realloc");
	}

	name = pattern = strdup(abbrev);
	/* Check for "name @ pattern" */
	for (p = pattern; *p; p++)
		if (*p == '@') {
			char *atsign = p;
			/* skip trailing space of name */
			while (p > pattern && (*(p-1) == ' ' || *(p-1) == '\t'))
				p--;
			*p = '\0';
			/* skip leading space of new pattern */
			for (p = atsign + 1; *p; p++)
				if (*p != ' ' && *p != '\t')
					break;
			pattern = p;
			break;
		}
	patterns[npatterns].name = name;
	patterns[npatterns].pattern = pattern;
	npatterns++;
}

/* Load the abbreviations listed in the given file */
void
abbrev_add_file(filename, ignore_open_error)
	const char *filename;
	int ignore_open_error;
{
	FILE *f;
	char pattern[1024], *p, *q;

	f = fopen(filename, "r");
	if (!f) {
		if (ignore_open_error)
			return;
		err(1, "%s", filename);
	}
	while ((p = fgets(pattern, sizeof pattern, f)) != NULL) {
		/* Ignore leading space */
		while (*p == ' ' || *p == '\t')
			p++;
		/* Ignore commenst */
		if (*p == '#')
			continue;
		/* Search for end of string */
		for (q = p; q < pattern + sizeof pattern - 1 && *q; q++)
			if (*q == '\n')
				break;
		/* Ignore trailing space */
		while (q > p && (*(q-1) == ' ' || *(q-1) == '\t'))
			q--;
		/* Terminate string */
		*q = '\0';
		abbrev_add(p);
	}
	if (ferror(f))
		warn("%s", filename);
	fclose(f);
}
