//
// Created by Ming Yao on 6/29/18.
//

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "utils.h"

int arr_sum(int n, const int *arr) {
    int res = 0;
    for (int i = 0; i < n; i++)
        res += arr[i];
    return res;
}

int arr_max(int n, const int *arr) {
    assert(n > 0);
    int res = arr[0];
    for (int i = 1; i < n; i++)
        res = arr[i] < res ? res : arr[i];
    return res;
}

char * _strdup(const char * src) {
    char *str;
    size_t len = strlen(src);

    str = (char *) malloc(len + 1);
    if (str) {
        memcpy(str, src, len + 1);
    }
    return str;
}

//char *
//_strsep (char **str, char delim)
//{
//    char *begin, *end, *cur;
//    cur = begin = *str;
//    if (begin == NULL)
//        return NULL;
//    /* Find the end of the token.  */
//    while(*cur != delim || *cur != '\0') {
//        cur++;
//    }
//    end =
//    end = begin + strcspn (begin, delim);
//    if (*end)
//    {
//        /* Terminate the token and set *STRINGP past NUL character.  */
//        *end++ = '\0';
//        *str = end;
//    }
//    else
//        /* No more delimiters; this is the last token.  */
//        *str = NULL;
//    return begin;
//}

//long _getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {
//    char c, *cur_pos, *new_lineptr;
//    size_t new_lineptr_len;
//
//    if (lineptr == NULL || n == NULL || stream == NULL) {
//        errno = EINVAL;
//        return -1;
//    }
//
//    if (*lineptr == NULL) {
//        *n = 128; /* init len */
//        if ((*lineptr = (char *)malloc(*n)) == NULL) {
//            errno = ENOMEM;
//            return -1;
//        }
//    }
//
//    cur_pos = *lineptr;
//    for (;;) {
//        c = (char) getc(stream);
//
//        if (ferror(stream) || (c == EOF && cur_pos == *lineptr))
//            return -1;
//
//        if (c == EOF)
//            break;
//
//        if ((*lineptr + *n - cur_pos) < 2) {
//            if (LONG_MAX / 2 < *n) {
//#ifdef EOVERFLOW
//                errno = EOVERFLOW;
//#else
//                errno = ERANGE; /* no EOVERFLOW defined */
//#endif
//                return -1;
//            }
//            new_lineptr_len = *n * 2;
//
//            if ((new_lineptr = (char *)realloc(*lineptr, new_lineptr_len)) == NULL) {
//                errno = ENOMEM;
//                return -1;
//            }
//            cur_pos = new_lineptr + (cur_pos - *lineptr);
//            *lineptr = new_lineptr;
//            *n = new_lineptr_len;
//        }
//
//        *cur_pos++ = c;
//
//        if (c == delim)
//            break;
//    }
//
//    *cur_pos = '\0';
//    return (long)(cur_pos - *lineptr);
//}
//
//long _getline(char **lineptr, size_t *n, FILE *stream) {
//    return _getdelim(lineptr, n, '\n', stream);
//}