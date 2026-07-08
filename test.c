#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>


typedef struct {
    int sitting;
    void (*sit)(void);
} dog_t;

__attribute__((noinline))
void sit_template(void)
{
    dog_t *dog = (dog_t *)0x1234567812345678ULL;
    dog->sitting = !dog->sitting;
}

void sit_template_end(void)
{
}

typedef void (*dog_method_t)(void);

dog_method_t make_method(dog_t *dog)
{
    size_t size =
        (uintptr_t)sit_template_end -
        (uintptr_t)sit_template;

    unsigned char *code = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    memcpy(code, sit_template, size);

    uint64_t needle = 0x1234567812345678ULL;
    uint64_t replacement = (uint64_t)dog;

    for (size_t i = 0; i < size - sizeof(uint64_t); i++) {
        uint64_t *p = (uint64_t *)(code + i);

        if (*p == needle) {
            *p = replacement;
            break;
        }
    }

    return (dog_method_t)code;
}

void main(void){
    dog_t dog1 = {0};
    dog_t dog2 = {0};

    dog1.sit = make_method(&dog1);
    dog2.sit = make_method(&dog2);

    dog1.sit();

    printf("%d\n", dog1.sitting);
    printf("%d\n", dog2.sitting);



}