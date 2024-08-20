// Copyright (c) 2001-2019  Pavel Rychly, Milos Jakubicek, Pavel Smerk

#ifdef USE_CREATEFILE
#undef USE_CREATEFILE
#endif

#include "writelex.hh"
#include "binfile.hh"
#include "log.hh"
#include "util.hh"
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <limits>

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

extern "C" {
#include <fsa3/writefsa.h>
}

using namespace std;

// -------------------- write_lexicon ------------------------

write_lexicon::write_lexicon (const string &filename, int maxitems, bool app)
    : filename (filename), cache(maxitems), added(maxitems),
      lovff (NULL), lex (NULL), append (app), cache_was_cleared (false),
      added_was_cleared (false), max_added_items (maxitems * 0.8),
      max_cache_items (maxitems * 0.8)
{
    lidxf = new ToFile<lexpos>(filename + ".lex.idx", append);
    lsrtf = new ToFile<lexpos>(filename + ".lex.srt", append);
    nextid = lidxf->tell();
    if (nextid > 0) {
        // ensure correct data files
        int lsrt_size = lsrtf->tell();
        delete lidxf;
        delete lsrtf;
        if (nextid != lsrt_size) {
            // corrupted .lex.srt
            CERR << "lexicon (" << filename
                 << ") datafiles corrupted, repairing ..." << endl;
            {
                ToFile<lexpos> tf ((filename + ".lex.srt").c_str());
                for (int i = 0; i < nextid; i++)
                    tf(i);
            } // free tf
            make_lex_srt_file (filename);
        }
        lidxf = new ToFile<lexpos>(filename + ".lex.idx", true);
        lsrtf = new ToFile<lexpos>(filename + ".lex.srt", true);
    }

    lexf = fopen ((filename + ".lex").c_str(), append ? "ab" : "wb");
    fseek (lexf, 0, SEEK_END);
    lexftell = ftell (lexf);
    if (nextid > 0)
        lex = new_lexicon(filename);
    last_lexf_ovf = numeric_limits<lexpos>::max();
    while(lexftell > last_lexf_ovf)
        last_lexf_ovf += numeric_limits<lexpos>::max();
}

write_lexicon::~write_lexicon ()
{
    flush_hash();
    delete lovff;
}

int write_lexicon::str2id(const char *str)
{
    if (cache.size() > max_cache_items) {
        cache.clear();
        cache_was_cleared = true;
    }
    cache_queries++;
    int_1 &idref = cache [str];
    if (idref.i == -1) {
        int id;
        cache_miss++;
        std::unordered_map<std::string,int>::const_iterator i;
        i = added.find(str);
        if (i != added.end()) {
            id = i->second;
        } else if (!lex || (id = lex->str2id (str)) < 0) {
            id = new_item (str);
            added [str] = id;
        }
        idref.i = id;
    }
    return idref.i;
}

int write_lexicon::new_item (const char *str)
{
    int len = strlen (str);
    int id = nextid++;
    lidxf->put (lexftell);
    if (lexftell > last_lexf_ovf) {
        if (!lovff)
            lovff = new ToFile<lexpos>(filename + ".lex.ovf", append);
        lovff->put (id);
        lovff->flush();
        last_lexf_ovf += numeric_limits<lexpos>::max();
    }
    lsrtf->put (id);
    fwrite (str, len +1, 1, lexf);
    lexftell += len +1;
    if (added.size() > max_added_items) {
        added_was_cleared = true;
        flush_hash();
        lex = new_lexicon(filename);
        lexf = fopen ((filename + ".lex").c_str(), "ab");
        lidxf = new ToFile<lexpos>(filename + ".lex.idx", true);
        lsrtf = new ToFile<lexpos>(filename + ".lex.srt", true);
    }
    return id;
}

int write_lexicon::avg_str_size ()
{
    return ceil(float(lexftell) / nextid) + sizeof(std::string);
}

float write_lexicon::pop_cache_miss_ratio()
{
    if (!cache_queries || !cache_was_cleared)
        return 0.5; // = do nothing
    float ret = float(cache_miss) / cache_queries;
    cache_miss = cache_queries = cache_was_cleared = 0;
    return ret;
}

int write_lexicon::pop_added_load()
{
    if (added_was_cleared || added.size() > 0.8 * max_added_items) {
        added_was_cleared = false;
        return 1;
    }
    if (added.size() < 0.2 * max_added_items)
        return -1;
    return 0;
}

struct compare_lex_items {
    lexicon *lex;
    compare_lex_items (lexicon *l) :lex (l) {}
    inline bool operator() (const int &x, const int &y)
    {
        return strcmp (lex->id2str (x), lex->id2str (y)) < 0;
    }
};


int make_lex_srt_file (const string &filename)
{
    CERR << "lexicon (" << filename
         << ") make_lex_srt_file\n";
    int srtsize;
    struct stat st;
    string fname = filename + ".lex.srt";
    if (stat (fname.c_str(), &st) < 0)
        throw FileAccessError (fname, "make_lex_srt_file:stat");
    srtsize = st.st_size / sizeof (int);
    int *srtfile;

#ifdef HAVE_MMAP
    int fd = open (fname.c_str(), O_RDWR);
    srtfile = (int *)mmap (0, st.st_size, PROT_READ|PROT_WRITE, 
                           MAP_SHARED, fd, 0);
    if (srtfile == MAP_FAILED)
        throw FileAccessError (fname, "make_lex_srt_file:mmap");
    close (fd);
#else // HAVE_MMAP
    FILE *f = fopen (fname.c_str(), "r+b");
    if (!f)
        throw FileAccessError (fname, "make_lex_srt_file:fopen");
    srtfile = new int [srtsize];
    if (!srtfile)
        throw FileAccessError (fname, "make_lex_srt_file:new");
    if (fread (srtfile, 1, st.st_size, f) < (size_t) st.st_size)
        throw FileAccessError (fname, "make_lex_srt_file:fread");
#endif

    lexicon *tmplex = new_lexicon (filename);
    compare_lex_items lexsrt (tmplex);
    sort (srtfile, srtfile + srtsize, lexsrt);
    delete tmplex;

#ifdef HAVE_MMAP
    if (msync ((void *) srtfile, st.st_size, MS_SYNC) == -1)
        throw FileAccessError (fname, "make_lex_srt_file:msync"); 
    if (munmap ((void *) srtfile, st.st_size) == -1)
        throw FileAccessError (fname, "make_lex_srt_file:munmap"); 
#else // HAVE_MMAP
    if (fseeko (f, 0, SEEK_SET))
        throw FileAccessError (fname, "make_lex_srt_file:fseek");
    if (fwrite (srtfile, 1, st.st_size, f) < (size_t) st.st_size)
        throw FileAccessError (fname, "make_lex_srt_file:fwrite");
    fclose (f);
    delete srtfile;
#endif

    return srtsize;
}

void write_lexicon::flush_hash()
{
    if (lex) {
        delete lex;
        lex = NULL;
    }
    fclose (lexf);
    delete lsrtf;
    delete lidxf;
    if (!added.empty()) {
        int srtsize = make_lex_srt_file (filename);
        if (srtsize != nextid)
            CERR << "incorrect lex size: srtsize=" << srtsize
                 << " nextid=" << nextid << endl;
    }
    added.clear();
}

// ------------------ write_unique_lexicon ---------------------

write_unique_lexicon::write_unique_lexicon (const string &filename)
    : filename (filename), lovff (NULL)
{
    lidxf = new ToFile<lexpos>(filename + ".lex.idx", true);
    nextid = lidxf->tell();
    lexf = fopen ((filename + ".lex").c_str(), "ab");
    fseek (lexf, 0, SEEK_END);
    lexftell = ftell (lexf);
    last_lexf_ovf = numeric_limits<lexpos>::max();
}

write_unique_lexicon::~write_unique_lexicon ()
{
    fclose (lexf);
    delete lidxf;
    delete lovff;
    {
    // create .lex.srt from scratch
    ToFile<lexpos> tf ((filename + ".lex.srt").c_str());
    for (int i = 0; i < nextid; i++)
        tf(i);
    tf.flush();
    } // calls tf destructor
    make_lex_srt_file (filename);
}

int write_unique_lexicon::str2id (const char *str)
{
    int len = strlen (str);
    int id = nextid++;
    lidxf->put (lexftell);
    if (lexftell > last_lexf_ovf) {
        if (!lovff)
            lovff = new ToFile<lexpos>(filename + ".lex.ovf", true);
        lovff->put (id);
        lovff->flush();
        last_lexf_ovf += numeric_limits<lexpos>::max();
    }
    fwrite (str, len +1, 1, lexf);
    lexftell += len +1;
    return id;
}

// ------------------ write_fsalex ---------------------

write_fsalex::write_fsalex (const string &filename, bool write_lexf, bool append)
    : filename (filename), hash (0), nextid (1),
      write_lexfiles (write_lexf), lexftell (0), oldlex (0),
      lovff (0), last_lexf_ovf (numeric_limits<lexpos>::max())
{
    if (append) {
        try {
            oldlex = new_lexicon (filename);
            nextid = oldlex->size();
        } catch (FileAccessError&) {
            this->open_outfiles();
        }
    } else this->open_outfiles();
}

void write_fsalex::open_outfiles (void)
{
    hash = hattrie_create();

    if (write_lexfiles) {
        lexf = fopen((filename + ".lex.tmp").c_str(), "wb");
        lidxf = new ToFile<lexpos>(filename + ".lex.idx.tmp");
    }

    lsrtf = new ToFile<lexpos>(filename + ".lex.srt.tmp");

    if (oldlex) {
        lexicon *lex = oldlex;
        oldlex = 0;
        nextid = 1;
        for (int id = 0; id < lex->size(); ++id)
            str2id(lex->id2str(id));
        delete lex;
    }
}

write_fsalex::~write_fsalex ()
{
    CERR << "lexicon (" << filename << ") writing FSA...\n";
    if (oldlex) {
        delete oldlex;
        CERR << "Writing FSA finished\n";
        return;
    }
    hattrie_iter_t *it = hattrie_iter_begin (hash, true);
    size_t len;
    fsm fsa;
    fsm_init (&fsa);
    while (! hattrie_iter_finished (it)) {
        unsigned char *word = (unsigned char *) hattrie_iter_key (it, &len);
        lsrtf->put ((unsigned int) *hattrie_iter_val (it) - 1);
        add_word (&fsa, word, len-1);
        hattrie_iter_next (it);
    }
    hattrie_iter_free (it);
    hattrie_free (hash);

    // write and rename .lex, .lex.idx
    if (write_lexfiles) {
        fclose (lexf);
        checked_rename((filename + ".lex.tmp").c_str(), (filename + ".lex").c_str());
        delete lidxf;
        checked_rename((filename + ".lex.idx.tmp").c_str(), (filename + ".lex.idx").c_str());
    }

    // write and rename .srt
    delete lsrtf;
    checked_rename((filename + ".lex.srt.tmp").c_str(), (filename + ".lex.srt").c_str());

    // write and rename .isrt
    FromFile <lexpos> ff_lsrtf (filename + ".lex.srt");
    lexpos *isrtf = new lexpos[nextid];
    int idx = 0;
    while (! ff_lsrtf)
        isrtf[ff_lsrtf.get()] = idx++;
    ToFile<lexpos> *lisrtf = new ToFile<lexpos> (filename + ".lex.isrt.tmp");
    for (int i = 0; i < idx; i++)
        lisrtf->put(isrtf[i]);
    delete[] isrtf;
    delete lisrtf;
    checked_rename((filename + ".lex.isrt.tmp").c_str(), (filename + ".lex.isrt").c_str());

    if (lovff) {
        delete lovff;
        checked_rename((filename + ".lex.ovf.tmp").c_str(), (filename + ".lex.ovf").c_str());
    } else unlink((filename + ".lex.ovf").c_str());

    // write and rename .fsa
    FILE *f = fopen ((filename + ".fsa.tmp").c_str(), "wb");
    if (write_fsa (&fsa, f)) {
        CERR << "Writing FSA failed\n";
        return;
    }
    fclose (f);
    checked_rename((filename + ".fsa.tmp").c_str(), (filename + ".fsa").c_str());
    CERR << "Writing FSA finished\n";
}

int write_fsalex::str2id (const char *str)
{
    size_t len = strlen (str) + 1;
    bool free_str = false;
    // hat-trie cannot store keys longer than 32767, details at https://github.com/dcjones/hat-trie/issues/30
    if (len > 32767ul) {
        len = 32767ul;
        str = strndup (str, len-1);
        free_str = true;
    }
    if (oldlex) {
        int id = oldlex->str2id(str);
        if (id != -1) {
            if (free_str) free ((void *) str);
            return id;
        }
        else open_outfiles();
    }
    value_t *tmp = hattrie_get (hash, str, len);
    if (! *tmp) { // new item
        *tmp = nextid++;
        if (write_lexfiles) {
            fwrite (str, len, 1, lexf);
            lidxf->put (lexftell);
            if (lexftell > last_lexf_ovf) {
                if (!lovff)
                    lovff = new ToFile<lexpos>(filename + ".lex.ovf.tmp", false);
                lovff->put (*tmp - 1);
                last_lexf_ovf += numeric_limits<lexpos>::max();
            }
            lexftell += len;
        }
    }
    if (free_str) free ((void *) str);
    return *tmp - 1;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
