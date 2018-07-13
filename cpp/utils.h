//
// Created by Ming Yao on 6/29/18.
//

#ifndef SBRL_UTILS_H
#define SBRL_UTILS_H

#include <stdio.h>

int arr_sum(int n, const int *arr);
int arr_max(int n, const int *arr);
char * _strdup(const char * src);
char * _strsep (char **stringp, const char *delim);
long _getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
long _getline(char **lineptr, size_t *n, FILE *stream);

#endif //SBRL_UTILS_H
