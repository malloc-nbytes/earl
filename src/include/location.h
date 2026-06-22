#ifndef LOCATION_H_INCLUDED
#define LOCATION_H_INCLUDED

#include "sv.h"

typedef struct {
        size_t r, c;
        sv path;
} location;

location location_from(size_t r, size_t c, sv path);

#endif // LOCATION_H_INCLUDED
