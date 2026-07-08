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
    void *caller        = (void *)magic_call_needle;\
    void (* fn)(void *) = (void *)magic_func_needle;\
    fn(caller);\
}\
void GGG_##name##_end(void){}

#define bind_method(caller, name)\
make_method(caller, name, GGG_##name##_method_wrapper, (size_t)((char *)GGG_##name##_end - (char *)GGG_##name##_method_wrapper) )




#define magic_call_needle 0x1234567812345678ULL
#define magic_func_needle 0x8765432187654321ULL
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
method_t make_method(void *caller, void *function, void *wrapper, size_t size){

    unsigned char *code = method_alloc(size);
    memcpy(code, wrapper, size);
    uint64_t replacement = (uint64_t)caller;

    for(size_t i = 0; i <= size - sizeof(uint64_t); i++){
        uint64_t *p = (uint64_t *)(code + i);

        if(*p == magic_call_needle){
            *p = (uint64_t)replacement;
        }else if (*p == magic_func_needle)
            *p = (uint64_t)function;
        }

    return (method_t)code;
}



typedef struct dog_data{
    char name[16];
    int sitting;
    void (*sit)(void);
    void (*stand)(void);
    void (*stats)(void);
} dog_t;

void sit  (dog_t *self);
void stand(dog_t *self);
void stats(dog_t *self);

def_method(sit)
void sit(dog_t *self){
    self->sitting = 1;
}

def_method(stand)
void stand(dog_t *self){
    self->sitting = 0;
}

def_method(stats)
void stats(dog_t *self){
    puts("dog status:");
    printf("name:\t%s\n",self->name);
    printf("state:\t%s\n\n",self->sitting?"sitting":"standing");
}

void construct_dog(dog_t *dog){
    dog->sit   = bind_method(dog, sit);
    dog->stand = bind_method(dog, stand);
    dog->stats = bind_method(dog, stats);
}

void main(void){

    dog_t dog1 = {0};
    construct_dog(&dog1);

    dog_t dog2 = {0};
    construct_dog(&dog2);

    strcpy(dog1.name, "rover");
    strcpy(dog2.name, "fido");

    dog1.stats();
    dog2.stats();
    
    dog1.sit();

    dog1.stats();
    dog2.stats();

    sleep(50);

}