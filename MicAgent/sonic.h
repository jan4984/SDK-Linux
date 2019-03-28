#ifndef _IFLYOS_SONIC_H_
#define _IFLYOS_SONIC_H_

#include "quiet.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sonic_receiver_t {
    quiet_decoder *dec;
};

int sonic_receiver_create(
        struct sonic_receiver_t *rec);

char *sonic_receiver_consume(
        struct sonic_receiver_t *rec,
        float *buf, size_t len);

void sonic_receiver_destroy(
        struct sonic_receiver_t *rec);

size_t sonic_speak(char *digits, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif // _IFLYOS_SONIC_H_
