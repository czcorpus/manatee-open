//  Copyright (c) 1999-2020  Pavel Rychly, Milos Jakubicek

#include "corpus.hh"
#include "config.hh"
#include "util.hh"
#ifdef SKETCH_ENGINE
#include "ngram.hh"
#include "sketch/wmap.hh"
#endif
#include "fromtof.hh"
#include <limits>
#include <math.h>
#include <algorithm>
#include <cstring>
#include <functional>

struct DocFreq {
    NumOfPos freq, docnum;
    DocFreq () : freq(0), docnum(0) {}
    operator NumOfPos() const {return freq;}
};

struct StarFreq {
    double sum;
    NumOfPos freq, docnum;
    StarFreq () : sum(0.0), freq(0), docnum(0) {}
    operator float() const {return (freq ? sum/double(freq) : 0);}
};

struct RedFreq { // Reduced Frequency
    double sum;
    Position prev, first;
    RedFreq () : sum(0.0), prev(-1), first(-1) {}
    operator float() const {return sum;}
};

template <typename FreqArray, typename ValueType, typename Value64Type>
void write_freqs (NumOfPos size, const string &path, FreqArray *freqs, int mapsize, MapBinFile<uint32_t> *mapid) {
    ToFile<ValueType> *freqfile = new ToFile<ValueType> (path + ".tmp");
    ToFile<Value64Type> *freq64file = NULL;
    if (mapid) {
        FreqArray *new_freqs = new FreqArray[mapsize]();  // the () causes zero-initialization
        for (NumOfPos id = 0; id < size; id++)
            new_freqs[(*mapid)[id]] = freqs[id];
        delete[] freqs;
        freqs = new_freqs;
        size = mapsize;
    }
    for (NumOfPos id = 0; id < size; id++) {
        Value64Type freq = freqs[id];
        if (freq > numeric_limits<ValueType>::max()) {
            delete freqfile;
            freqfile = NULL;
            FromFile<ValueType> frq32 (path + ".tmp");
            freq64file = new ToFile<Value64Type> (path + "64.tmp");
            while (!frq32)
                freq64file->put (frq32.get());
            freq64file->put(freq);
        } else
            freqfile ? freqfile->put(freq) : freq64file->put(freq);
    }
    delete freqfile; delete freq64file; delete[] freqs;
    if (freqfile)
        checked_rename ((path + ".tmp").c_str(), path.c_str());
    else // freq64file
        checked_rename ((path + "64.tmp").c_str(), (path + "64").c_str());
    delete mapid;
}

WordList * open_attr (const char *attr, Corpus *c, int *mapsize,
                      MapBinFile<uint32_t> **mapid, bool *shouldDelete) {
#ifdef SKETCH_ENGINE
    *shouldDelete = true;
    size_t len = strlen (attr);
    if (!strcmp(attr + len - 4, ".ngr")) {
        NGram *ngr = new NGram (c->get_conf("PATH") + attr);
        *mapsize = ngr->size();
        *mapid = new MapBinFile<uint32_t> (c->get_conf("PATH") + attr + ".wl.nid");
        return ngr->get_wordlist(true);
    } else if (!strcmp(attr, "WSCOLLOC")) {
        WMap *wm = new_WMap (c->get_conf("WSBASE"), c->get_confpath());
        return wm->get_wordlist(true);
    } else if (!strcmp(attr, "TERM")) {
        const string &termbase = c->get_conf ("TERMBASE");
        // XXX backward compatibility
        size_t len = termbase.size();
        if (len > 3 && !strcmp (termbase.c_str() + len - 3, "-ws")) {
            return c->get_wordlist(termbase.substr(0, len-3));
        }
        return c->get_wordlist(termbase);
    }
#endif
    *shouldDelete = false;
    WordList *pa = c->get_attr (attr);
    auto ao = c->conf->find_attr(attr);
    bool multival = c->conf->str2bool (ao["MULTIVALUE"]);
    if (multival)
        pa->prepare_multivalue_map(ao["MULTISEP"].c_str());
    return pa;
}

void Corpus::compile_aldf (const char *attr) {
    int mapsize = 0;
    MapBinFile<uint32_t> *mapid = NULL;
    bool shouldDelete;
    WordList *wl = open_attr (attr, this, &mapsize, &mapid, &shouldDelete);
    IDPosIterator *it = wl->idposat(0);
    string path = get_conf("PATH");
    if (!get_conf("SUBCPATH").empty()) { // = is subcorpus
        path = get_conf("SUBCPATH");
        it = filter_idpos (it);
    }
    path += wl->name + ".aldf";

    RedFreq * freqs = new RedFreq[wl->id_range()](); // the () causes zero-initialization
    NumOfPos pos_count = search_size();
    double size = pos_count;
    NumOfPos prog_step, curr_prog, i = 0;
    prog_step = curr_prog = pos_count / 100;
    fprintf (stderr, "\r0 %%");
    Position last_pos = -1;
    while (!it->end()) {
        if (i > curr_prog) {
            fprintf (stderr, "\r%d %%", int (100 * i / pos_count));
            curr_prog += prog_step;
        }
        Position pos = it->peek_pos() - it->get_delta(); // recalculate position (for subcorpora)
        if (pos > last_pos) {
            i++;
            last_pos = pos;
        }
        int32_t id = it->peek_id();
        if (id != -1)
            wl->expand_multivalue_id(id, [&freqs, &pos, &size](int nid){
                RedFreq *aldf = &freqs[nid];
                if (aldf->prev == -1) // first hit
                    aldf->first = aldf->prev = pos;
                else {
                    if (pos > aldf->prev) {  // workaround for terms starting at the same position
                        double dn = (pos - aldf->prev) / size;
                        aldf->sum += dn * log2 (dn);
                    }
                    aldf->prev = pos;
                }
            });
        it->next();
    }
    // the first/last interval is treated as a cyclic one
    for (NumOfPos id = 0; id < wl->id_range(); id++) {
        RedFreq *aldf = &freqs[id];
        if (aldf->prev == -1) // never occurred
            continue;
        double first_last = (aldf->first + size - aldf->prev) / size;
        aldf->sum += first_last * log2 (first_last);
        aldf->sum = exp2 (-aldf->sum);
    }
    fprintf (stderr, "\r100 %%\n");
    delete it;

    write_freqs<RedFreq, float, double> (wl->id_range(), path, freqs, mapsize, mapid);
    if (shouldDelete)
        delete wl;
}

void Corpus::compile_arf (const char *attr) {
    int mapsize = 0;
    MapBinFile<uint32_t> *mapid = NULL;
    bool shouldDelete;
    WordList *wl = open_attr (attr, this, &mapsize, &mapid, &shouldDelete);
    IDPosIterator *it = wl->idposat(0);
    string path = get_conf("PATH");
    if (!get_conf("SUBCPATH").empty()) { // = is subcorpus
        path = get_conf("SUBCPATH");
        it = filter_idpos (it);
    }
    path += wl->name + ".arf";

    RedFreq * freqs = new RedFreq[wl->id_range()](); // the () causes zero-initialization
    Frequency *frq = wl->get_stat("frq");
    NumOfPos pos_count = search_size();
    double size = pos_count;
    NumOfPos prog_step, curr_prog, i = 0;
    prog_step = curr_prog = pos_count / 100;
    fprintf (stderr, "\r0 %%");
    Position last_pos = -1;
    while (!it->end()) {
        if (i > curr_prog) {
            fprintf (stderr, "\r%d %%", int (100 * i / pos_count));
            curr_prog += prog_step;

        }
        Position pos = it->peek_pos() - it->get_delta(); // recalculate position (for subcorpora)
        if (pos > last_pos) {
            i++;
            last_pos = pos;
        }
        int32_t id = it->peek_id();
        if (id != -1)
            wl->expand_multivalue_id(id, [&freqs, &size, &wl, &pos, &frq](int nid){
                double v = size / frq->freq(nid); // window size
                RedFreq *arff = &freqs[nid];
                if (arff->prev == -1) // first hit
                    arff->first = arff->prev = pos;
                else {
                    double d = pos - arff->prev;
                    arff->prev = pos;
                    if (d < v)
                        arff->sum += d / v;
                    else
                        arff->sum += 1.0;
                }
            });
        it->next();
    }
    // the first/last interval is treated as a cyclic one
    for (NumOfPos id = 0; id < wl->id_range(); id++) {
        RedFreq *arff = &freqs[id];
        if (arff->prev == -1) // never occurred
            continue;
        double v = size / frq->freq(id); // window size
        Position first_last = arff->first + size - arff->prev;
        if (first_last < v)
            arff->sum += first_last / v;
        else
            arff->sum += 1.0;
    }
    fprintf (stderr, "\r100 %%\n");
    delete it; delete frq;

    write_freqs<RedFreq, float, double> (wl->id_range(), path, freqs, mapsize, mapid);
    if (shouldDelete)
        delete wl;
}

void Corpus::compile_frq (const char *attr) {
    int mapsize = 0;
    MapBinFile<uint32_t> *mapid = NULL;
    bool shouldDelete;
    WordList *wl = open_attr (attr, this, &mapsize, &mapid, &shouldDelete);
    IDPosIterator *it = wl->idposat(0);
    string path = get_conf("PATH");
    if (!get_conf("SUBCPATH").empty()) { // = is subcorpus
        path = get_conf("SUBCPATH");
        it = filter_idpos (it);
    }
    path += wl->name + ".frq";

    NumOfPos * freqs = new NumOfPos[wl->id_range()](); // the () causes zero-initialization
    NumOfPos pos_count = search_size();
    NumOfPos prog_step, curr_prog, i = 0;
    prog_step = curr_prog = pos_count / 100;
    fprintf (stderr, "\r0 %%");
    Position last_pos = -1;
    while (!it->end()) {
        if (i > curr_prog) {
            fprintf (stderr, "\r%d %%", int (100 * i / pos_count));
            curr_prog += prog_step;
        }
        Position pos = it->peek_pos();
        if (pos > last_pos) {
            i++;
            last_pos = pos;
        }
        int32_t id = it->peek_id();
        if (id != -1)
            wl->expand_multivalue_id(id, [&freqs](int nid){
                freqs[nid]++;
            });
        it->next();
    }
    fprintf (stderr, "\r100 %%\n");
    delete it;

    write_freqs<NumOfPos, uint32_t, int64_t> (wl->id_range(), path, freqs, mapsize, mapid);
    if (shouldDelete)
        delete wl;
}

void Corpus::compile_docf (const char *attr, const char *docstruc) {
    int mapsize = 0;
    MapBinFile<uint32_t> *mapid = NULL;
    bool shouldDelete;
    WordList *wl = open_attr (attr, this, &mapsize, &mapid, &shouldDelete);
    IDPosIterator *it = wl->idposat(0);
    RangeStream *struc = get_struct (docstruc)->rng->whole();
    string path = get_conf("PATH");
    if (!get_conf("SUBCPATH").empty()) { // = is subcorpus
        path = get_conf("SUBCPATH");
        it = filter_idpos (it);
        struc = filter_query (struc);
    }
    path += wl->name + ".docf";

    DocFreq * freqs = new DocFreq[wl->id_range()](); // the () causes zero-initialization
    NumOfPos pos_count = search_size();
    NumOfPos docnum = 1;
    Position next_end = struc->peek_end();
    NumOfPos prog_step, curr_prog, i = 0;
    prog_step = curr_prog = pos_count / 100;
    fprintf (stderr, "\r0 %%");
    Position last_pos = -1;
    while (!it->end()) {
        if (i > curr_prog) {
            fprintf (stderr, "\r%d %%", int (100 * i / pos_count));
            curr_prog += prog_step;
        }
        Position pos = it->peek_pos();
        if (pos > last_pos) {
            i++;
            last_pos = pos;
        }
        if (pos >= next_end) {
            docnum++;
            struc->next();
            next_end = struc->peek_end();
        }
        int32_t id = it->peek_id();
        if (id != -1) {  // avoid holes between structures
            wl->expand_multivalue_id(id, [&freqs, &docnum](int nid){
                DocFreq *df = &freqs[nid];
                if (df->docnum < docnum) {
                    df->freq++;
                    df->docnum = docnum;
                }
            });
        }
        it->next();
    }
    fprintf (stderr, "\r100 %%\n");
    delete it; delete struc;

    write_freqs<DocFreq, uint32_t, int64_t> (wl->id_range(), path, freqs, mapsize, mapid);
    if (shouldDelete)
        delete wl;
}

void Corpus::compile_star (const char *attr, const char *docstruc, const char *structattr) {
    int mapsize = 0;
    MapBinFile<uint32_t> *mapid = NULL;
    bool shouldDelete;
    WordList *wl = open_attr (attr, this, &mapsize, &mapid, &shouldDelete);
    IDPosIterator *it = wl->idposat(0);
    Structure *s = get_struct(docstruc);
    PosAttr *sa = s->get_attr(structattr);
    RangeStream *struc = s->rng->whole();
    string path = get_conf("PATH");
    if (!get_conf("SUBCPATH").empty()) { // = is subcorpus
        path = get_conf("SUBCPATH");
        it = filter_idpos (it);
        struc = filter_query (struc);
    }
    path += wl->name + ".star";

    StarFreq * freqs = new StarFreq[wl->id_range()](); // the () causes zero-initialization
    NumOfPos pos_count = search_size();
    NumOfPos docnum = 1;
    Position next_end = struc->peek_end();
    NumOfPos prog_step, curr_prog, i = 0;
    prog_step = curr_prog = pos_count / 100;
    fprintf (stderr, "\r0 %%");
    Position last_pos = -1;
    while (!it->end()) {
        if (i > curr_prog) {
            fprintf (stderr, "\r%d %%", int (100 * i / pos_count));
            curr_prog += prog_step;
        }
        Position pos = it->peek_pos();
        if (pos > last_pos) {
            i++;
            last_pos = pos;
        }
        if (pos >= next_end) {
            docnum++;
            struc->next();
            next_end = struc->peek_end();
        }
        int32_t id = it->peek_id();
        if (id >= 0) {  // avoid holes between documents
            wl->expand_multivalue_id(id, [&freqs, &docnum, &sa](int nid){
                StarFreq *df = &freqs[nid];
                if (df->docnum < docnum) {
                    char *errcheck;
                    double val = strtod (sa->pos2str(docnum - 1), &errcheck);
                    if (*errcheck == '\0') {
                        df->freq++;
                        df->sum += val;
                    }
                    df->docnum = docnum;
                }
            });
        }
        it->next();
    }
    fprintf (stderr, "\r100 %%\n");
    delete it; delete struc;

    write_freqs<StarFreq, float, double> (wl->id_range(), path, freqs, mapsize, mapid);
    if (shouldDelete)
        delete wl;
}

void calc_average_structattr (Corpus *c, const char *structname, const char *attrname, RangeStream *q,
                              double &avg, NumOfPos &docf)
{
    Structure *s = c->get_struct(structname);
    PosAttr *sa = s->get_attr(attrname);
    FastStream *f = s->part_nums(q);
    double sum = 0;
    docf = 0;
    char *errcheck;
    while (f->peek() < f->final()) {
        double val = strtod (sa->pos2str(f->peek()), &errcheck);
        if (*errcheck == '\0') {
            sum += val;
            docf++;
        }
        f->next();
    }
    delete f;
    if (docf)
        avg = sum/double(docf);
    else
        avg = 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
