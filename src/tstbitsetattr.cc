// Test for BitsetPosAttr (pabitset.hh) on a small generated attribute

#include "pabitset.hh"
#include "consumer.hh"
#include "writelex.hh"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <dirent.h>
#include <strings.h>

using namespace std;

static int failures = 0;

#define CHECK(cond, msg) \
    do { if (!(cond)) { cerr << "FAIL: " << msg << endl; failures++; } } while (0)

typedef vector<string> StrSet;

// simplified UD-like FEATS values, including an empty set, unsorted input
// and a repeated key
static const vector<StrSet> pattern = {
    {"Case=Nom", "Number=Sing"},
    {},
    {"Number=Plur"},
    {"Case=Acc", "Number=Sing", "Gender=Fem"},
    {"Number=Sing", "Case=Nom"},
    {"Case=Nom", "Case=Acc"},
    {},
    {"Gender=Fem", "Gender=Fem"},
};

// what the encoder (work item 4) is expected to produce: lexicon of values
// (plus "" for empty sets), list_text of value ids, reverse index with
// "" postings for positions with an empty set
static void write_bitset_attr (const string &base, const vector<StrSet> &data)
{
    write_fsalex lex (base, true, false);
    ListTextConsumer lt (base + ".ltext");
    RevFileConsumer *rev = RevFileConsumer::create (base);
    int empty_id = lex.str2id ("");
    lex.str2id ("Unused=Yes"); // value without any position
    vector<int> ids;
    for (size_t pos = 0; pos < data.size(); pos++) {
        ids.clear();
        for (const string &s : data[pos])
            ids.push_back (lex.str2id (s.c_str()));
        sort (ids.begin(), ids.end());
        ids.erase (unique (ids.begin(), ids.end()), ids.end());
        lt.put (ids);
        if (ids.empty())
            rev->put (empty_id, pos);
        for (int id : ids)
            rev->put (id, pos);
    }
    delete rev;
}

static vector<Position> stream2vec (FastStream *fs)
{
    vector<Position> v;
    while (fs->peek() < fs->final())
        v.push_back (fs->next());
    delete fs;
    return v;
}

static void remove_dir (const string &dir)
{
    DIR *d = opendir (dir.c_str());
    if (!d)
        return;
    while (struct dirent *e = readdir (d)) {
        string n = e->d_name;
        if (n != "." && n != "..")
            unlink ((dir + "/" + n).c_str());
    }
    closedir (d);
    rmdir (dir.c_str());
}

static void run_tests (const string &dir)
{
    const NumOfPos n = 1000; // several list_text segments
    vector<StrSet> data;
    for (NumOfPos p = 0; p < n; p++)
        data.push_back (pattern [p % pattern.size()]);
    string base = dir + "/feats";
    write_bitset_attr (base, data);

    PosAttr *pa = createBitsetPosAttr (base, "feats", "", "UTF-8");
    CHECK (pa->size() == n, "size " << pa->size());

    // expected sets as sorted id vectors
    vector<vector<int> > expected (n);
    map<int, vector<Position> > poss_of;
    for (NumOfPos p = 0; p < n; p++) {
        for (const string &s : data[p]) {
            int id = pa->str2id (s.c_str());
            CHECK (id >= 0, "value missing in lexicon: " << s);
            expected[p].push_back (id);
        }
        sort (expected[p].begin(), expected[p].end());
        expected[p].erase (unique (expected[p].begin(), expected[p].end()),
                           expected[p].end());
        for (int id : expected[p])
            poss_of[id].push_back (p);
    }
    int empty_id = pa->str2id ("");
    CHECK (empty_id >= 0, "\"\" missing in lexicon");

    auto joined = [&] (const vector<int> &ids) {
        vector<string> vals;
        for (int id : ids)
            vals.push_back (pa->id2str (id));
        sort (vals.begin(), vals.end(), [] (const string &a, const string &b) {
            int c = strcasecmp (a.c_str(), b.c_str());
            return c ? c < 0 : a < b;
        });
        string s;
        for (size_t i = 0; i < vals.size(); i++)
            s += (i ? "|" : "") + vals[i];
        return s;
    };

    // random access
    vector<int> got;
    for (NumOfPos p = 0; p < n; p++) {
        pa->posset (p, got);
        CHECK (got == expected[p], "posset at " << p);
        CHECK (pa->pos2str (p) == joined (expected[p]),
               "pos2str at " << p << ": '" << pa->pos2str (p) << "'");
        int rep = expected[p].empty() ? empty_id : expected[p][0];
        CHECK (pa->pos2id (p) == rep, "pos2id at " << p);
    }
    // non-sequential access (posset() caches its reading position)
    for (NumOfPos p = n - 1; p >= 0; p -= 3) {
        pa->posset (p, got);
        CHECK (got == expected[p], "posset backwards at " << p);
    }
    srand (7);
    for (int k = 0; k < 2000; k++) {
        NumOfPos p = rand() % n;
        pa->posset (p, got);
        CHECK (got == expected[p], "posset random at " << p);
        if (p + 1 < n) {
            pa->posset (p + 1, got);
            CHECK (got == expected[p + 1], "posset next after random " << p);
        }
    }
    CHECK (string (pa->pos2str (0)).find ('|') != string::npos,
           "pos2str does not join values: " << pa->pos2str (0));

    // sequential iterators, from the start and from mid-segment
    for (Position start : {Position (0), Position (37), Position (n - 3)}) {
        TextIterator *ti = pa->textat (start);
        IDIterator *ii = pa->posat (start);
        for (Position p = start; p < n; p++) {
            int rep = expected[p].empty() ? empty_id : expected[p][0];
            CHECK (ti->next() == joined (expected[p]),
                   "textat(" << start << ") at " << p);
            CHECK (ii->next() == rep, "posat(" << start << ") at " << p);
        }
        CHECK (ii->next() == -1, "posat past end");
        delete ti;
        delete ii;
    }

    // (position, id) pairs, an empty set yields the id of ""
    for (Position start : {Position (0), Position (37)}) {
        IDPosIterator *it = pa->idposat (start);
        vector<pair<Position,int> > pairs, exp_pairs;
        for (; !it->end(); it->next())
            pairs.push_back (make_pair (it->peek_pos(), it->peek_id()));
        delete it;
        for (Position p = start; p < n; p++) {
            if (expected[p].empty())
                exp_pairs.push_back (make_pair (p, empty_id));
            for (int id : expected[p])
                exp_pairs.push_back (make_pair (p, id));
        }
        CHECK (pairs == exp_pairs, "idposat(" << start << "): "
               << pairs.size() << " pairs, expected " << exp_pairs.size());
    }

    // reverse index: every value finds exactly its positions
    for (auto &po : poss_of) {
        string name = pa->id2str (po.first);
        CHECK (stream2vec (pa->id2poss (po.first)) == po.second,
               "id2poss " << name);
        CHECK (pa->freq (po.first) == NumOfPos (po.second.size()),
               "freq " << name << ": " << pa->freq (po.first));
    }
    vector<Position> empty_poss;
    for (NumOfPos p = 0; p < n; p++)
        if (expected[p].empty())
            empty_poss.push_back (p);
    CHECK (stream2vec (pa->id2poss (empty_id)) == empty_poss,
           "id2poss of \"\" (positions with an empty set)");
    int unused = pa->str2id ("Unused=Yes");
    CHECK (unused >= 0 && stream2vec (pa->id2poss (unused)).empty()
           && pa->freq (unused) == 0, "value without positions");

    // regex over individual values: any position having some Case value
    set<Position> with_case;
    for (NumOfPos p = 0; p < n; p++)
        for (const string &s : data[p])
            if (s.compare (0, 5, "Case=") == 0)
                with_case.insert (p);
    CHECK (stream2vec (pa->regexp2poss ("Case=.*", false))
           == vector<Position> (with_case.begin(), with_case.end()),
           "regexp2poss Case=.*");

    delete pa;
}

int main ()
{
    char tmpl[] = "/tmp/tstbitsetattr.XXXXXX";
    if (!mkdtemp (tmpl)) {
        perror ("mkdtemp");
        return 1;
    }
    string dir = tmpl;
    try {
        run_tests (dir);
    } catch (exception &e) {
        cerr << "FAIL: exception: " << e.what() << endl;
        failures++;
    }
    remove_dir (dir);
    if (failures) {
        cerr << failures << " check(s) failed" << endl;
        return 1;
    }
    cout << "OK" << endl;
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
