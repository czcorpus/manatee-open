#include <hat-trie/hat-trie.h>
#include "stdio.h"

/* skeindex(slovo) vrací buď existující, nebo nové číslo, čísluje se od jedničky (nula je tam, kde žádné slovo nekončí) */

hattrie_t* ske;

unsigned int num = 0;
static inline unsigned int skeindex(const char *word, size_t length) {
    value_t *tmp;
    tmp = hattrie_get(ske, word, length);
    if (! *tmp) *tmp = ++num;
    return *tmp;
}


/* testovací/ukázkový kód + kód pro tvorbu .lex, .lex.idx a .lex.srt */
int main(const int argc, const char *argv[]) {
    char *word = NULL;
    size_t buffer;
    ssize_t length;
    hattrie_iter_t *ske_iter;
    unsigned int old = 0, pos = 0;

    FILE *lex, *srt, *idx;
    lex = fopen("hat.lex", "wb");
    idx = fopen("hat.lex.idx", "wb");
    srt = fopen("hat.lex.srt", "wb");
    
    ske = hattrie_create();
    while ((length = getline(&word, &buffer, stdin)) != -1) {
        if (word[length - 1] == '\n') word[length - 1] = '\0';
        if (! *word) continue;
        /* fprintf(stdout, "%s\t%d\n", word, skeindex(word, (size_t) length - 1) - 1); // ladění */
        skeindex(word, (size_t) length - 1);
        if (old != num) {
            fwrite(word, (size_t) length, 1, lex);
            fwrite(&pos, sizeof(unsigned int), 1, idx);
            pos += length;
            old = num;
        }
    }
    free(word);
    fclose(lex);
    fclose(idx);

    if (argc > 1 && *argv[1] == 'x') return 0;

    /* nějaký setvbuf? Nebylo by nejlepší to narvat do paměti a pak vyplivnout zaráz? */
    ske_iter = hattrie_iter_begin(ske, true);
    while (! hattrie_iter_finished(ske_iter)) {
        value_t tmp = *hattrie_iter_val(ske_iter) - 1;
        /* fprintf(stdout, "%s\t%ld\n", hattrie_iter_key(ske_iter, &buffer), *hattrie_iter_val(ske_iter)); // ladění */
        fwrite(&tmp, sizeof(value_t), 1, srt);
        hattrie_iter_next(ske_iter);
    }
    hattrie_iter_free(ske_iter);
    fclose(srt);

    hattrie_free(ske);
    return 0;
}
