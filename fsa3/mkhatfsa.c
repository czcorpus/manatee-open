#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <hat-trie/hat-trie.h>
#include "writefsa.h"

#include "time.h"

hattrie_t* ske;

unsigned int num = 0;
static inline unsigned int skeindex(const char *word, size_t length) {
    value_t *tmp;
    tmp = hattrie_get(ske, word, length);
    if (! *tmp) *tmp = ++num;
    return *tmp;
}

int main() {
    char *word = NULL;
    size_t buffer;
    ssize_t length;
    hattrie_iter_t *ske_iter;
    size_t len;
    fsm autom;
    int error;
time_t rawtime;

    ske = hattrie_create();

time(&rawtime); fprintf(stderr, "Start time: %s", ctime(&rawtime));

    while ((length = getline(&word, &buffer, stdin)) != -1) {
        if (word[length - 1] == '\n') word[--length] = '\0';
        if (! *word) continue;
        skeindex(word, (size_t) length);
    }
    free(word);

    fsm_init(&autom);
    ske_iter = hattrie_iter_begin(ske, true);

time(&rawtime); fprintf(stderr, "Before fsa: %s", ctime(&rawtime));

    while (! hattrie_iter_finished(ske_iter)) {
        word = (char *) hattrie_iter_key(ske_iter, &len);
        add_word(&autom, (unsigned char *) word, len);
        hattrie_iter_next(ske_iter);
    }
    hattrie_iter_free(ske_iter);

    hattrie_free(ske);
    if ((error = write_fsa(&autom, stdout))) {
        fprintf(stderr, "Could not write the automaton\n");
        return error;
    }

time(&rawtime); fprintf(stderr, "After work: %s", ctime(&rawtime));

    return 0;
}
