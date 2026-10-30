// Owned constructor-state reproducer. No address is patched or executed.
#include "InterceptEntry.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <new>
#ifndef EXPECT_REPAIRED
#error Define EXPECT_REPAIRED as0 or1.
#endif
static unsigned checks=0;
static void check(bool value,const char *message){++checks;if(!value){std::fprintf(stderr,"FAILED %s\n",message);std::abort();}}
int main(){
    alignas(8) unsigned char ownedDataToken[8]{};
    const addr_t even=reinterpret_cast<addr_t>(ownedDataToken);
    check((even&1U)==0,"owned data-token alignment");
    unsigned reproduced=0,repaired=0;
    for(unsigned char fill : {static_cast<unsigned char>(0xa5),static_cast<unsigned char>(0x01)}){
        for(bool odd : {false,true}){
            alignas(InterceptEntry) unsigned char storage[sizeof(InterceptEntry)];
            std::memset(storage,fill,sizeof storage);
            auto *entry=new(static_cast<void *>(storage)) InterceptEntry(kFunctionInlineHook,even+(odd?1U:0U));
            unsigned char representation=0;
            std::memcpy(&representation,storage+offsetof(InterceptEntry,thumb_mode),1);
            // Inspect object representation only. Never evaluate the unfixed bool.
            const unsigned expected=odd?1U:(EXPECT_REPAIRED?0U:fill);
            check(representation==expected,"exact initialized/untouched mode representation");
            check(entry->patched_addr==even,"odd-token normalization preserves owned address");
            check(entry->type==kFunctionInlineHook,"constructor type assignment");
            if(!odd){if(EXPECT_REPAIRED)++repaired;else ++reproduced;}
            entry->~InterceptEntry();
        }
    }
    std::printf("{\"passed\":true,\"checks\":%u,\"placement_cases\":4,\"expected_repaired\":%s,\"uninitialized_reproductions\":%u,\"initialized_false_cases\":%u,\"uninitialized_bool_evaluated\":false,\"hook_called\":false}\n",
        checks,EXPECT_REPAIRED?"true":"false",reproduced,repaired);
}
