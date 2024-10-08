#ifndef UTILS_H
#define UTILS_H
	#include <stddef.h>
void crash_game(char* msg);
	void init_glfw();
	void init_glad();
	typedef struct buffer
	{
		void *data;
		size_t size;
	} buffer;
#endif
