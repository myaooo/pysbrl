//
// Created by Ming Yao on 6/29/18.
//

#include <assert.h>
#include <stdlib.h>
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
    char *p;
    size_t len = strlen(src);

    str = malloc(len + 1);
    if (str) {
        memcpy(str, src, len + 1);
    }
    return str;
}