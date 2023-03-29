// Copyright (c) 1999-2022  Pavel Rychly, Milos Jakubicek

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
    regexp_pattern *pat = new regexp_pattern (pattern, locale, encoding, ignorecase);
    Generator<int> *ids;
    vector<string> *parts = pat->disjoint_parts();

    if (pat->any()) {
        ids = new SequenceGenerator<int> (0, lex->size() -1);
    } else if (pat->no_meta_chars() && !ignorecase) {
        // pattern is no regexp, pat.prefix() has been de-escaped
        int id = lex->str2id (pat->prefix());
        delete pat;
        if (id < 0)
            return new EmptyGenerator<int>();
        else
            ids = new SequenceGenerator<int> (id, id);
    } else if (parts->size() && !ignorecase) {
        // pattern has form (word1|word2|word3|...)
        int *str_ids = new int[parts->size()];
        size_t i, j;
        for (i = 0, j = 0; i < parts->size(); i++) {
            int id = lex->str2id ((*parts)[i].c_str());
            if (id >= 0)
                str_ids[j++] = id;
        }
        switch (j) {
        case 0:
            delete[] str_ids;
            ids = new EmptyGenerator<int>();
            break;
        case 1:
            ids = new SequenceGenerator<int> (str_ids[0], str_ids[0]);
            delete[] str_ids;
            break;
        default:
            std::sort(str_ids, str_ids + j);
            ids = new ArrayGenerator<int> (str_ids, j);
        }
    } else {
        if (pat->compile()) {
            delete pat;
            return new EmptyGenerator<int>();
        }
        if (ignorecase || strlen(pat->prefix()) == 0)
            ids = new SequenceGenerator<int> (0, lex->size() -1);
        else {
            if (filter_fs) {
                if (filter_fs->peek() < filter_fs->final())
                    ids = new Fast2Gen<int> (filter_fs);
                else {
                    delete pat;
                    return new EmptyGenerator<int>();
                }
            }
            else {
                ids = lex->pref2ids (pat->prefix());
                if (ids->end()) {
                    delete pat;
                    delete ids;
                    return new EmptyGenerator<int>();
                }
            }
        }
        ids = new regexp2idsStream (lex, ids, pat);
    }

    if (filter_pattern && filter_pattern[0] != '\0') {
        regexp_pattern *filter = new regexp_pattern (filter_pattern, locale, encoding, ignorecase);
        if (filter->compile()) {
            delete filter;
            return new EmptyGenerator<int>();
        }
        return new regexp2idsStream (lex, ids, filter, true);
    } else {
        return ids;
    }
}

IdStrGenerator *regexp2strids (lexicon *lex, const char *pattern,
        const char *locale, const char *encoding,
        bool ignorecase, const char *filter_pattern, FastStream *filter_fs)
{
    regexp_pattern *pat = new regexp_pattern (pattern, locale, encoding, ignorecase);
    IdStrGenerator *ids;
    vector<string> *parts = pat->disjoint_parts();

    if (pat->any()) {
        ids = lex->pref2strids("");
    } else if (pat->no_meta_chars() && !ignorecase) {
        // pattern is no regexp, pat.prefix() has been de-escaped
        const char *pref = pat->prefix();
        int id = lex->str2id (pref);
        if (id < 0) {
            delete pat;
            return new EmptyIdStrGenerator();
        }
        ids = new SingleIdStrGenerator (id, pref);
        delete pat; // must not delete pat earlier -- "pref" no longer available after deleting pat
    } else if (parts->size() && !ignorecase) {
        // pattern has form (word1|word2|word3|...)
        int *str_ids = new int[parts->size()];
        size_t i;
        for (i = 0; i < parts->size(); i++) {
            int id = lex->str2id ((*parts)[i].c_str());
            if (id >= 0)
                str_ids[i] = id;
        }
        switch (i) {
        case 0:
            delete[] str_ids;
            ids = new EmptyIdStrGenerator();
        case 1: {
            ids = new SingleIdStrGenerator (str_ids[0], (*parts)[0].c_str());
            delete[] str_ids;
        }
        default:
            ids = new ArrayIdStrGenerator (parts, str_ids);
        }
    } else {
        if (pat->compile()) {
            delete pat;
            return new EmptyIdStrGenerator();
        }
        if (ignorecase || strlen(pat->prefix()) == 0)
            ids = lex->pref2strids("");
        else {
            if (filter_fs) {
                if (filter_fs->peek() < filter_fs->final())
                    ids = new AddStr<lexicon>(new Fast2Gen<int> (filter_fs), lex);
                else {
                    delete pat;
                    return new EmptyIdStrGenerator();
                }
            }
            else {
                //cerr << "prefix=" << pat->prefix() << "\n";
                ids = lex->pref2strids (pat->prefix());
                if (ids->end()) {
                    delete pat;
                    delete ids;
                    return new EmptyIdStrGenerator();
                }
            }
        }
        ids = new regexp2idstrStream (lex, ids, pat);
    }

    if (filter_pattern) {
        regexp_pattern *filter = new regexp_pattern (filter_pattern, locale, encoding, ignorecase);
        if (filter->compile()) {
            delete filter;
            return new EmptyIdStrGenerator();
        }
        return new regexp2idstrStream (lex, ids, filter, true);
    } else {
      return ids;
    }
}
// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
