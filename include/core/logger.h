// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

extern int g_verbose;

#define LOG_INFO(...)  do { if (g_verbose) fprintf(stderr, __VA_ARGS__); } while (0)
#define LOG_ERROR(...) do { fprintf(stderr, __VA_ARGS__); } while (0)

#endif