#include "location.h"

location
location_from(size_t r,
              size_t c,
              sv     path)
{
        return (location) {
                .r = r,
                .c = c,
                .path = path,
        };
}