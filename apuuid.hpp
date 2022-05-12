#ifndef _APUUID_H
#define _APUUID_H


#include <stdint.h>
#include <stdio.h>
#include <string>


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include <stdlib.h>
#include <time.h>
#endif


static void apuuid_init_rng()
{
    #ifdef __EMSCRIPTEN__
    /* js crypto needs no init from C */
    #else
    srand ((unsigned int) time (NULL));
    #endif
}

static uint8_t apuuid_rand_byte()
{
    #ifdef __EMSCRIPTEN__
    return (uint8_t)EM_ASM_INT({
        var buf = new Uint8Array(1);
        crypto.getRandomValues(buf);
        return buf[0];
    });
    #else
    return rand();
    #endif
}

static void apuuid_generate(char* out)
{
    for (uint8_t i=0; i<16; i++) {
        sprintf(out + 2*i, "%02hhx", apuuid_rand_byte());
    }
}

static std::string ap_get_uuid(const std::string& uuidFile)
{
    char uuid[33]; uuid[32] = 0;
#if defined USE_IDBFS || !defined __EMSCRIPTEN__
    FILE* f = uuidFile.empty() ? nullptr : fopen(uuidFile.c_str(), "rb");
    size_t n = 0;
    if (f) {
        n = fread(uuid, 1, 32, f);
        fclose(f);
    }
    if (!f || n < 32) {
        apuuid_init_rng();
        apuuid_generate(uuid);
        f = uuidFile.empty() ? nullptr : fopen(uuidFile.c_str(), "wb");
        if (f) {
            n = fwrite(uuid, 1, 32, f);
            fclose(f);
            #ifdef __EMSCRIPTEN__
            EM_ASM(
                FS.syncfs(function (err) {});
            );
            #endif
        }
        if (!uuidFile.empty() && (!f || n < 32)) {
            fprintf(stderr, "Could not write persistant UUID!\n");
        }
    }
    f = nullptr;
#else
    #error TODO: implement localStorage API
#endif
    return uuid;
}

#endif // _APUUID_H
