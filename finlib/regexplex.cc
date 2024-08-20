// Copyright (c) 1999-2023  Pavel Rychly, Milos Jakubicek

#include <finlib/regexplex.hh>
#include <algorithm>
#include <iostream>

class regexp2idsStream : public Generator<int>
{
protected:
    lexicon *lex;
    Generator<int> *lexit;
    regexp_pattern *pat;
    int currid;
    bool finished;
    bool negative;
    void locate () {
        finished = true;
        while (!lexit->end()) {
            currid = lexit->next();
            if (pat->match (lex->id2str (currid)) == !negative) {
                finished = false;
                break;
            }
        }
    }
public:
    regexp2idsStream (lexicon *l, Generator<int> *li, regexp_pattern *p, bool negative = false)
        : lex (l), lexit (li), pat (p), finished (false), negative(negative) {
        locate();
    }
    virtual int next() {
        int ret = currid;
        locate();
        return ret;
    }
    virtual bool end() {return finished;}
    virtual const int size() const {return lexit->size();}
    virtual ~regexp2idsStream() {delete lexit; delete pat;}
};

class regexp2idstrStream : public IdStrGenerator
{
protected:
    lexicon *lex;
    IdStrGenerator *lexit;
    regexp_pattern *pat;
    int currid;
    string currstr;
    bool finished;
    bool negative;
    void locate () {
        finished = true;
        while (!lexit->end()) {
            currstr = lexit->getStr();
            currid = lexit->getId();
            lexit->next();
            if (pat->match (currstr.c_str()) == !negative) {
                finished = false;
                break;
            }
        }
    }
public:
    regexp2idstrStream (lexicon *l, IdStrGenerator *li, regexp_pattern *p, bool negative = false)
        : lex (l), lexit (li), pat (p), finished (false), negative(negative) {
        locate();
    }
    virtual void next() {locate();}
    virtual int getId() {return currid;}
    virtual string getStr() {return currstr;}
    virtual bool end() {return finished;}
    virtual const int size() const {return lexit->size();}
    virtual ~regexp2idstrStream() {delete lexit; delete pat;}
};


Generator<int> *regexp2ids (lexicon *lex, const char *pattern,
        const char *locale, const char *encoding,
        bool ignorecase, const char *filter_pattern, FastStream *filter_fs)
{
    unique_ptr<FastStream> filter_fs_ptr(filter_fs);
    unique_ptr<regexp_pattern> pat(new regexp_pattern (pattern, locale, encoding, ignorecase));
    unique_ptr<Generator<int> > ids;
    vector<string> *parts = pat->disjoint_parts();

    if (pat->any()) {
        ids.reset(new SequenceGenerator<int> (0, lex->size() -1));
    } else if (pat->no_meta_chars() && !ignorecase) {
        // pattern is no regexp, pat.prefix() has been de-escaped
        int id = lex->str2id (pat->prefix());
        if (id < 0)
            return new EmptyGenerator<int>();
        else
            ids.reset(new SequenceGenerator<int> (id, id));
    } else if (parts->size() && !ignorecase) {
        // pattern has form (word1|word2|word3|...)
        unique_ptr<int[]> str_ids(new int[parts->size()]);
        size_t i, j;
        for (i = 0, j = 0; i < parts->size(); i++) {
            int id = lex->str2id ((*parts)[i].c_str());
            if (id >= 0)
                str_ids[j++] = id;
        }
        switch (j) {
        case 0:
            ids.reset(new EmptyGenerator<int>());
            break;
        case 1:
            ids.reset(new SequenceGenerator<int> (str_ids[0], str_ids[0]));
            break;
        default:
            std::sort(str_ids.get(), str_ids.get() + j);
            ids.reset(new ArrayGenerator<int> (str_ids.release(), j));
        }
    } else {
        if (pat->compile())
            return new EmptyGenerator<int>();
        if (ignorecase || strlen(pat->prefix()) == 0)
            ids.reset(new SequenceGenerator<int> (0, lex->size() -1));
        else {
            if (filter_fs) {
                if (filter_fs->peek() < filter_fs->final())
                    ids.reset(new Fast2Gen<int> (filter_fs_ptr.release()));
                else
                    return new EmptyGenerator<int>();
            }
            else {
                ids.reset(lex->pref2ids (pat->prefix()));
                if (ids->end())
                    return new EmptyGenerator<int>();
            }
        }
        ids.reset(new regexp2idsStream (lex, ids.release(), pat.release()));
    }

    if (filter_pattern && filter_pattern[0] != '\0') {
        regexp_pattern *filter = new regexp_pattern (filter_pattern, locale, encoding, ignorecase);
        if (filter->compile()) {
            delete filter;
            return new EmptyGenerator<int>();
        }
        return new regexp2idsStream (lex, ids.release(), filter, true);
    } else {
        return ids.release();
    }
}

IdStrGenerator *regexp2strids (lexicon *lex, const char *pattern,
        const char *locale, const char *encoding,
        bool ignorecase, const char *filter_pattern, FastStream *filter_fs)
{
    unique_ptr<FastStream> filter_fs_ptr(filter_fs);
    unique_ptr<regexp_pattern> pat(new regexp_pattern (pattern, locale, encoding, ignorecase));
    unique_ptr<IdStrGenerator> ids;
    vector<string> *parts = pat->disjoint_parts();

    if (pat->any()) {
        ids.reset(lex->pref2strids(""));
    } else if (pat->no_meta_chars() && !ignorecase) {
        // pattern is no regexp, pat.prefix() has been de-escaped
        const char *pref = pat->prefix();
        int id = lex->str2id (pref);
        if (id < 0)
            return new EmptyIdStrGenerator();
        ids.reset(new SingleIdStrGenerator (id, pref));
    } else if (parts->size() && !ignorecase) {
        // pattern has form (word1|word2|word3|...)
        unique_ptr<int[]> str_ids(new int[parts->size()]);
        size_t i, j;
        for (i = 0, j = 0; i < parts->size(); i++) {
            int id = lex->str2id ((*parts)[i].c_str());
            if (id >= 0)
                str_ids[j++] = id;
        }
        switch (j) {
        case 0:
            ids.reset(new EmptyIdStrGenerator());
        case 1: {
            ids.reset(new SingleIdStrGenerator (str_ids[0], (*parts)[0].c_str()));
        }
        default:
            std::sort(str_ids.get(), str_ids.get() + j);
            ids.reset(new AddStr<lexicon>(new ArrayGenerator<int> (str_ids.release(), j), lex));
        }
    } else {
        if (pat->compile())
            return new EmptyIdStrGenerator();
        if (ignorecase || strlen(pat->prefix()) == 0)
            ids.reset(lex->pref2strids(""));
        else {
            if (filter_fs) {
                if (filter_fs->peek() < filter_fs->final())
                    ids.reset(new AddStr<lexicon>(new Fast2Gen<int> (filter_fs_ptr.release()), lex));
                else
                    return new EmptyIdStrGenerator();
            }
            else {
                //cerr << "prefix=" << pat->prefix() << "\n";
                ids.reset(lex->pref2strids (pat->prefix()));
                if (ids->end())
                    return new EmptyIdStrGenerator();
            }
        }
        ids.reset(new regexp2idstrStream (lex, ids.release(), pat.release()));
    }

    if (filter_pattern) {
        regexp_pattern *filter = new regexp_pattern (filter_pattern, locale, encoding, ignorecase);
        if (filter->compile()) {
            delete filter;
            return new EmptyIdStrGenerator();
        }
        return new regexp2idstrStream (lex, ids.release(), filter, true);
    } else {
      return ids.release();
    }
}
// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
