#include "c_utils.h"
#include "controls.h"

#include <cstring>
#include <cstdio>

//SDL_Keycode is typedef Sint32 = int32_t so
//values should probably be int32_t ...

int parse_config_file(const char* filename, const char** keys, int* values, int num_keys)
{
	c_array out;

	if (!file_open_read(filename, "r", &out)) {
		printf("Could not open/read file\n");
		perror(NULL);
		return 0;
	}

	char delims[] = "\n";
	char* result = NULL;
	char* tmp = NULL;
	char map;
	result = strtok((char*)out.data, delims );
	while (result != NULL) {
		printf( "result is \"%s\"", result);
		tmp = strchr(result, ':');
		if (tmp) {
			*tmp = '\0';
			tmp++;
			while (*tmp++ != '\'');
			map = *tmp++;
			printf("\n%s\t%c\n", result, map);
			if (*tmp == '\'') {
				for (int i=0; i<num_keys; i++) {
					if (!strcmp(keys[i], result)) {
						//printf("  %c\t%u\t%u\n", map, map, keymap[map]);
						//values[i] = keymap[map];
						values[i] = map;
						break;
					}
				}
			}
		}

		result = strtok( NULL, delims );
	}

	free(out.data);

	return 1;
}
/*
int parse_config_file(const char* filename, const char** keys, sf::Key::Code* values)
{
	c_array out;

	if (!file_open_read(filename, "r", &out))
		return 0;

	char delims[] = "\n";
	char* result = NULL;
	char* tmp = NULL;
	char map;
	result = strtok((char*)out.data, delims );
	while (result != NULL) {
		printf( "result is \"%s\"", result);
		tmp = strchr(result, ':');
		if (tmp) {
			*tmp = '\0';
			tmp++;
			while (*tmp++ != '\'');
			map = *tmp++;
			printf("\n%s\t%c\n", result, map);
			if (*tmp == '\'') {
				for (int i=0; i<NCONTROLS; i++) {
					if (!strcmp(keys[i], result)) {
						printf("  %c\t%u\t%u\n", map, map, keymap[map]);
						values[i] = keymap[map];
						break;
					}
				}
			}
		}

		result = strtok( NULL, delims );
	}

	return 1;
}
*/
