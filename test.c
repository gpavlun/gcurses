#include <sys/mman.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define def_method(name) \
__attribute__((noinline)) \
void GGG_##name##_method_wrapper(void){\
    void *caller = (void *)magic_needle;\
    name(caller);\
}\
void GGG_##name##_end(void){}

#define bind_method(caller, name)\
make_method(caller, GGG_##name##_method_wrapper, (size_t)((char *)GGG_##name##_end - (char *)GGG_##name##_method_wrapper) )




#define magic_needle 0x1234567812345678ULL
#define METHOD_PAGE_SIZE 4096

typedef struct {
    void *start;
    size_t size;
} method_meta_t;

typedef struct {
    uint8_t *base;
    uint8_t *current;
    size_t remaining;
} method_page_t;

static method_page_t current_method_page = {0};

static void new_method_page(void){
    void *page = mmap(
        NULL,
        METHOD_PAGE_SIZE,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    page == MAP_FAILED ? ({
        perror("mmap");
        exit(1);
    }) : ({
        current_method_page.base = (uint8_t *)page;
        current_method_page.current = (uint8_t *)page;
        current_method_page.remaining = METHOD_PAGE_SIZE;
    });
}

static void seal_method_page(void){
    !current_method_page.base ? ({
        return;
    }) : 
    mprotect( current_method_page.base,
              METHOD_PAGE_SIZE, 
              PROT_READ | PROT_EXEC ) == -1 ? ({
        perror("mprotect");
        exit(1);
    }) : 0;
}

void *method_alloc(size_t size){
    size = (size + 15) & ~15;

    size > METHOD_PAGE_SIZE ? ({
        return NULL;
    }) : 
    !current_method_page.current || current_method_page.remaining < size ? ({
        seal_method_page();
        new_method_page();
    }): 0;

    void *result = current_method_page.current;

    current_method_page.current += size;
    current_method_page.remaining -= size;

    return result;
}


typedef void (*method_t)(void);
method_t make_method(void *caller, void *function, size_t size){

    unsigned char *code = method_alloc(size);
    memcpy(code, function, size);
    uint64_t replacement = (uint64_t)caller;

    for(size_t i = 0; i <= size - sizeof(uint64_t); i++){
        uint64_t *p = (uint64_t *)(code + i);

        if(*p == magic_needle){
            *p = replacement;
            break;
        }
    }

    return (method_t)code;
}



typedef struct dog_data{
    int sitting;
    void (*sit)(void);
} dog_t;

void sit(dog_t *self);


def_method(sit)
void sit(dog_t *self){
    self->sitting++;
}


#define DOG_COUNT 100000

void test_dogs(void)
{
    dog_t *dogs = malloc(sizeof(dog_t) * DOG_COUNT);

    if (!dogs) {
        perror("malloc");
        exit(1);
    }

    for (size_t i = 0; i < DOG_COUNT; i++)
    {
        dogs[i].sitting = 0;
        dogs[i].sit = bind_method(&dogs[i], sit);
    }

    // poke a few of them to verify they work
    for (size_t i = 0; i < DOG_COUNT; i++)
    {
        dogs[i].sit();
    }

    printf("dog[0]: %d\n", dogs[0].sitting);
    printf("dog[last]: %d\n", dogs[DOG_COUNT - 1].sitting);


    free(dogs);
}


void main(void){

    test_dogs();

    sleep(50);

}