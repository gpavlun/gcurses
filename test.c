#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>


typedef struct {
    int sitting;
    void (*sit)(void);
} dog_t;

typedef struct {
    void *start;
    size_t size;
} method_meta_t;

#define def_method(name, def) \
__attribute__((noinline)) \
def \
void name##_end(void){}


def_method(sit_template, 
    void sit_template(void){
        dog_t *dog = (dog_t *)0x1234567812345678ULL;
        dog->sitting = !dog->sitting;
    }
)


#define bind_method(caller, name)\
make_method(caller, name, (size_t)((char *)name##_end - (char *)name) )


typedef void (*method_t)(void);

method_t make_method(void *caller, void *function, size_t size)
{

    unsigned char *code = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    memcpy(code, function, size);

    uint64_t needle = 0x1234567812345678ULL;
    uint64_t replacement = (uint64_t)caller;

    for (size_t i = 0; i < size - sizeof(uint64_t); i++) {
        uint64_t *p = (uint64_t *)(code + i);

        if (*p == needle) {
            *p = replacement;
            break;
        }
    }

    return (method_t)code;
}

void main(void){
    dog_t dog1 = {0};
    dog_t dog2 = {0};

    dog1.sit = bind_method(&dog1, sit_template);
    dog2.sit = bind_method(&dog2, sit_template);

    dog1.sit();

    printf("%d\n", dog1.sitting);
    printf("%d\n", dog2.sitting);

}