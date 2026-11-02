// Round-trip test for list_text / ListTextConsumer

#include "consumer.hh"
#include "text.hh"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>

using namespace std;

static int failures = 0;

#define CHECK(cond, msg) \
    do { if (!(cond)) { cerr << "FAIL: " << msg << endl; failures++; } } while (0)

static string ids2str (const vector<int> &v)
{
    string s = "{";
    for (size_t i = 0; i < v.size(); i++)
        s += (i ? "," : "") + to_string (v[i]);
    return s + "}";
}

static void remove_files (const string &base)
{
    unlink ((base + ".ltext").c_str());
    unlink ((base + ".ltext.seg").c_str());
}

static void test_empty (const string &dir)
{
    string base = dir + "/empty";
    { ListTextConsumer lc (base + ".ltext"); }
    map_list_text lt (base);
    vector<int> out;
    CHECK (lt.size() == 0, "empty: size " << lt.size());
    CHECK (!lt.at(0).next (out), "empty: next() at 0 returned true");
    remove_files (base);
}

static void test_roundtrip (const string &dir, int seg_size, NumOfPos n,
                            unsigned seed)
{
    string base = dir + "/rt" + to_string (seg_size);
    srand (seed);
    vector<vector<int> > expected (n);
    {
        ListTextConsumer lc (base + ".ltext", seg_size);
        for (NumOfPos p = 0; p < n; p++) {
            vector<int> in;
            int kind = rand() % 10;
            if (kind >= 3) { // 30 % empty sets
                int cnt = 1 + rand() % 8;
                for (int i = 0; i < cnt; i++) {
                    int id = (kind == 9) ? rand() % (1 << 30)
                                         : rand() % 1000;
                    in.push_back (id);
                    if (rand() % 5 == 0)
                        in.push_back (id); // duplicate
                }
            }
            if (p % 1000 == 0)
                in.push_back (0); // smallest id
            lc.put (in); // unsorted input
            sort (in.begin(), in.end());
            in.erase (unique (in.begin(), in.end()), in.end());
            expected[p] = in;
        }
        CHECK (lc.curr_size() == n, "curr_size " << lc.curr_size());
    }

    map_list_text lt (base);
    string tag = "seg_size=" + to_string (seg_size) + ": ";
    CHECK (lt.size() == n, tag << "size " << lt.size() << " != " << n);
    vector<int> out;

    // random access to every position
    for (NumOfPos p = 0; p < n; p++) {
        lt.pos2ids (p, out);
        CHECK (out == expected[p], tag << "pos " << p << ": got "
               << ids2str (out) << " expected " << ids2str (expected[p]));
    }

    // sequential read of the whole text
    map_list_text::const_iterator it = lt.at (0);
    NumOfPos p = 0;
    while (it.next (out)) {
        CHECK (p < n && out == expected[p], tag << "sequential pos " << p);
        p++;
    }
    CHECK (p == n, tag << "sequential read stopped at " << p);

    // sequential reads starting mid-segment, crossing segment boundaries
    for (int k = 0; k < 50; k++) {
        NumOfPos start = rand() % n;
        it = lt.at (start);
        for (NumOfPos q = start; q < min (n, start + 3 * seg_size); q++) {
            bool ok = it.next (out);
            CHECK (ok && out == expected[q], tag << "from " << start
                   << " pos " << q);
        }
    }

    CHECK (!lt.at (n).next (out), tag << "next() at size returned true");
    CHECK (!lt.at (n + 100).next (out), tag << "next() past size returned true");
    remove_files (base);
}

static void test_negative_id (const string &dir)
{
    string base = dir + "/neg";
    bool thrown = false;
    {
        ListTextConsumer lc (base + ".ltext");
        try {
            lc.put (vector<int> {3, -1});
        } catch (invalid_argument &) {
            thrown = true;
        }
    }
    CHECK (thrown, "negative id not rejected");
    remove_files (base);
}

int main ()
{
    char tmpl[] = "/tmp/tstlisttext.XXXXXX";
    if (!mkdtemp (tmpl)) {
        perror ("mkdtemp");
        return 1;
    }
    string dir = tmpl;
    try {
        test_empty (dir);
        test_roundtrip (dir, 64, 100000, 1);
        test_roundtrip (dir, 7, 10000, 2);
        test_roundtrip (dir, 1, 1000, 3);
        test_negative_id (dir);
    } catch (exception &e) {
        cerr << "FAIL: exception: " << e.what() << endl;
        failures++;
    }
    rmdir (dir.c_str());
    if (failures) {
        cerr << failures << " check(s) failed" << endl;
        return 1;
    }
    cout << "OK" << endl;
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
