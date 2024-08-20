#define _GNU_SOURCE
#include <hat-trie/hat-trie.h>
#include <stdio.h>


// skeindex(slovo) vrací buď existující, nebo nové číslo, čísluje se od jedničky (nula je tam, kde žádné slovo nekončí)

hattrie_t* ske;

unsigned int num = 0;
static inline unsigned int skeindex(const char *word, size_t length) {
	value_t *tmp;
	tmp = hattrie_get(ske, word, length);
	if (! *tmp) *tmp = ++num;
	return *tmp;
}


int main(const int argc, const char *argv[]) {
	char *word = NULL;
	size_t buffer;
	ssize_t length;
	ske = hattrie_create();

	while ((length = getline(&word, &buffer, stdin)) != -1) {
		if (word[length - 1] == '\n') word[length - 1] = '\0';
		if (! *word) continue;
		skeindex(word, (size_t) length - 1);
	}
	free(word);

	if (argc > 1 && *argv[1] == 'x') return 0;

	hattrie_iter_t *ske_iter;
	ske_iter = hattrie_iter_begin(ske, true);
	const char * tmpword;
	size_t tmplen;
	while (! hattrie_iter_finished(ske_iter)) {
		tmpword = hattrie_iter_key(ske_iter, &tmplen);
		fprintf(stdout, "%s\n", tmpword);
		hattrie_iter_next(ske_iter);
	}
	hattrie_iter_free(ske_iter);

	hattrie_free(ske);
	return 0;
}
