#include "file_picker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH_LEN 1024

static void trim_right(char buffer[])
{
	int len = strlen(buffer);

	int cursor = len - 1;
	const char whitespace[] = " \n\r\t";
	while (cursor >= 0)
	{
		char ch = buffer[cursor];
		bool is_whitespace = false;
		for (int i = 0; i < strlen(whitespace); i++)
		{
			if (ch == whitespace[i])
				is_whitespace = true;
		}

		if (is_whitespace)
			buffer[cursor] = '\0';
		else
			break;

		cursor--;
	}
}

static bool check_zenity(void)
{
	FILE* zenity = popen("command -v zenity", "r");

	int first_char = fgetc(zenity);

	pclose(zenity);

	return first_char != EOF;
}

// Result must be freed
char* open_dialog(bool save_mode)
{
	if (!check_zenity())
	{
		fprintf(stderr, "ERROR: zenity is necessary to show dialog\n");
		return NULL;
	}

	FILE* zenity = NULL;
	if (save_mode)
		zenity = popen("zenity --file-selection --save", "r");
	else
		zenity = popen("zenity --file-selection", "r");

	if (!zenity)
	{
		fprintf(stderr, "ERROR: Failed to open zenity\n");
		return NULL;
	}

	char buffer[MAX_PATH_LEN];
	char* ok = fgets(buffer, 1024, zenity);
	pclose(zenity);

	// Canceled
	if (!ok)
		return NULL;

	int len = strlen(buffer);

	char* result = (char*)malloc(len + 1);
	strncpy(result, buffer, len);
	trim_right(result);

	return result;
}
