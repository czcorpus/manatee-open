// Copyright (c) 1999-2009  Pavel Rychly

#include <config.hh>
#include "srtruns.hh"
#include "bigram.hh"
#include <cstdio>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "message.hh"
#include "util.hh"

using namespace std;

#ifndef SECOND_TYPE
#define SECOND_TYPE int
//#define SECOND_TYPE pair<int,int>
#endif

static string numbered_name (const string &base, int num)
{
    if (num == -1)
        return base;
    char s[8];
    sprintf (s, "#%i", num);
    return base + s;
}
void joinbgrsrange (const string &base, int fromn, int ton);

// ==================== Sort Phase ====================

// -------------------- bgritem --------------------
struct bgritem {
    int first;
    SECOND_TYPE second;
    int count;
    bgritem (int i1, int c) : first (i1), count (c) {}
    bgritem () {}
    bool operator < (const bgritem &x) const { 
        return (first < x.first || (first == x.first && second < x.second));
    }
    bool operator == (const bgritem &x) const { 
        return first == x.first && second == x.second;
    }
};

//-------------------- tempbgr --------------------
class tempbgr {
    struct uniq_data {
        int fromnum;
        int filenum;
        const int minbgr;
        bgritem prev;
        FILE *idxf, *cntf;
        int nextid;
        bool nosum;
        int maxfiles;
        uniq_data (int fromnum=0, int minb=0, bool nos=false, int maxfiles=0)
            :fromnum (fromnum), filenum (fromnum), minbgr (minb), prev (-1,0),
             cntf (NULL), nextid (0), nosum (nos), maxfiles (maxfiles) {}
    };
    uniq_data *data;
    const string filename;
    bool origin;

    void open_next ();
    void write_idx(int newid);
    void write_bgr () {
        if (data->prev.count >= data->minbgr) {
            fwrite (&data->prev.second, sizeof (data->prev.second), 1, 
                    data->cntf);
            fwrite (&data->prev.count, sizeof (data->prev.count), 1, 
                    data->cntf);
        }
    }
public:
    tempbgr (const tempbgr &c) : data (c.data), filename (c.filename),
        origin (false) {}
    tempbgr (const string &filebase, bool nos=false, int maxfiles=0,
             int fromnum=0, int minbgr=0)
        : data (new uniq_data(fromnum, minbgr, nos, maxfiles)), 
          filename (filebase), origin (true)
        { open_next(); }
    ~tempbgr () {
        if (origin) {
            write_idx (data->prev.first +1); 
            fclose (data->cntf); fclose (data->idxf);
        }
    }
    void operator() (bgritem &x);
    int num_of_files() const {return data->filenum;}
    int fromnum() const {return data->fromnum;}
};

void tempbgr::open_next () 
{
    if (data->cntf) {
        fclose (data->cntf); fclose (data->idxf);
    }
    if (data->maxfiles && (data->filenum - data->fromnum == data->maxfiles)) {
        joinbgrsrange (filename, data->fromnum, data->filenum);
        data->fromnum = data->filenum++;
    }
    string s = numbered_name (filename, data->filenum);
    data->cntf = fopen ((s + ".cnt").c_str(), "wb");
    if (!data->cntf)
        throw FileAccessError ((s + ".cnt"), "tempbgr::fopen");
    data->idxf = fopen ((s + ".idx").c_str(), "wb");
    if (!data->idxf)
        throw FileAccessError ((s + ".idx"), "tempbgr::fopen");
    data->filenum ++;
}

void tempbgr::operator() (bgritem &x)
{
    if (data->prev.first != x.first) {
        write_idx (x.first);
        data->prev = x;
    } else if (data->nosum || x.second != data->prev.second) {
        write_bgr();
        data->prev = x;
    } else {
        data->prev.count += x.count;
    }
}

void tempbgr::write_idx (int newid)
{
    if (data->prev.first != -1)
        write_bgr();
    int seek = ftell (data->cntf) / 8;
    fwrite (&seek, sizeof (int), 1, data->idxf);
    if (data->prev.first > newid) {
        open_next();
        data->nextid = 0;
        seek = 0;
        fwrite (&seek, sizeof (int), 1, data->idxf);
    }
    while (data->nextid++ < newid)
        fwrite (&seek, sizeof (int), 1, data->idxf);
}

//==================== Join Phase ====================

struct joinbgritem {
    BigramItem bi;
    BigramStream *bs;
    joinbgritem (BigramStream *s): bi (**s), bs (s) {}
    joinbgritem () {}
    bool operator < (joinbgritem &x) {
        return (bi.first < x.bi.first) || 
            (bi.first == x.bi.first && bi.second < x.bi.second);
    }
};

typedef vector<BigramStream*> BgrSs;
typedef vector<joinbgritem> joinbgrvec;

void joinbgrsfiles (BgrSs &inbgrs, const string &outfilename, int minbgr,
                    bool nosum)
{
    tempbgr output (outfilename, nosum, 0, -1, minbgr);
    joinbgrvec jheap (inbgrs.size());

    int insert = inbgrs.size()-1;
    for (BgrSs::const_iterator i = inbgrs.begin(); insert >= 0; --insert, i++)
        add_to_heap<joinbgrvec::iterator,
                    int, joinbgritem> (jheap.begin(), insert, 
                                        jheap.size(), *i);

    while (!jheap.empty()) {
        joinbgrvec::iterator head = jheap.begin();
        bgritem bb (head->bi.first, head->bi.value);
        bb.second = head->bi.second;
        output (bb);
        ++(*head->bs);
        if (!*head->bs) {
            head->bi = **(head->bs);
            add_to_heap<joinbgrvec::iterator,int, joinbgritem> (head, 0, 
                                                        jheap.size(), *head);
        } else {
            add_to_heap<joinbgrvec::iterator,int, joinbgritem> (head, 0, 
                                                        jheap.size()-1, 
                                                        *(jheap.end() -1));
            jheap.pop_back();
        }
    }
}

void joinbgrsrange (const string &base, int fromn, int ton)
{
    // join parts
    BgrSs bss;
    int i = fromn;
    while (i < ton)
        bss.push_back (new BigramStream (numbered_name (base, i++)));

    joinbgrsfiles (bss, numbered_name (base, ton), 0, false);
    // remove temp files
    for (i = fromn; i < ton; i++) {
        string s = numbered_name (base, i);
        unlink ((s + ".cnt").c_str());
        unlink ((s + ".idx").c_str());
    }
}

static void rename_bgr_files (string frombase, string tobase)
{
    checked_rename((frombase + ".cnt").c_str(), (tobase + ".cnt").c_str());
    checked_rename((frombase + ".idx").c_str(), (tobase + ".idx").c_str());
}


//==================== main ====================

static void usage (const char *progname) {
    cerr << "usage: " << progname << " [OPTIONS] BIGRAM_PATH [FILENAME]\n"
        "    -b BUFF_SIZE    use BUFF_SIZE items buffer during sort phase\n" 
        "    -s              sort phase only\n"
        "    -j NUM_OF_FILES join phase only\n"
        "    -m MINBGR       store only bigrams greater or equal to MINBGR\n"
        "    -a              append to existing files\n"
        "    -d              no sum of duplicates\n"
        "    -p MAX_PARTS    maximum number of temporary parts\n"
        //      "    -t MAXITEMS     store only top MAXITEMS items\n"
        ;
}


int main (int argc, char **argv) 
{
    char *outname;
    int buff_size = 8 * 1024 * 1024;
    FILE *input = stdin;

    bool join_phase = true, sort_phase = true, append_to = false;
    int num_of_files = 0;
    int start_num = 0;
    int minbgr = 0;
//    int maxitems = 0;
    int maxparts = 0;
    bool nosum = false;
    const char *progname = argv[0];
    message msg (progname);

    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "h?sj:b:m:adp:t:")) != -1)
            switch (c) {
            case 's':
                join_phase = false;
                break;
            case 'j':
                sort_phase = false; 
                num_of_files = atol (optarg);
                break;
            case 'b':
                buff_size = atol (optarg);
                break;
            case 'm':
                minbgr = atol (optarg);
                break;
            case 'h':
            case '?':
                usage (progname);
                return 2;
            case 'a':
                append_to = true;
                break;
            case 'd':
                nosum = true;
                break;
            case 't':
//              maxitems = atol (optarg);
                break;
            case 'p':
                maxparts = atol (optarg);
                break;
            default:
                cerr << "unknown option (" << c << ")\n";
                usage (progname);
                return 2;
            }
        
        if (optind < argc) {
            outname = argv [optind++];
            if (optind < argc) {
                input = fopen (argv [optind], "r");
                if (!input)
                    throw FileAccessError (argv [optind], "mkbgr::fopen");
            }
        } else {
            usage (progname);
            return 2;
        }
    }
    msg.setup (progname, outname);

    try {
        if (sort_phase) {
            tempbgr oi (outname, nosum, maxparts);
            make_sorted_runs (FromFile<bgritem>(input), oi, buff_size);
            num_of_files = oi.num_of_files();
            start_num = oi.fromnum();
            msg ("stream sorted, #parts: %i", num_of_files);
        }

        if (join_phase) {
            if (num_of_files - start_num > 1 || append_to) {
                // join parts
                BgrSs bss;
                if (append_to) {
                    rename_bgr_files (outname,
                                      numbered_name (outname, num_of_files));
                    num_of_files++;
                }
                
                for (int i = start_num; i < num_of_files; i++) {
                    char s[1024];
                    sprintf (s, "%s#%i", outname, i);
                    bss.push_back (new BigramStream(s));
                }
                joinbgrsfiles (bss, outname, minbgr, nosum);
                msg ("parts joined");

                // remove temp files
                for (int i = start_num; i < num_of_files; i++) {
                    string s = numbered_name (outname, i);
                    unlink ((s + ".cnt").c_str());
                    unlink ((s + ".idx").c_str());
                }
                msg ("temporary files removed");
            } else {
                if (minbgr <= 1) {
                    // rename the only part
                    rename_bgr_files (numbered_name (outname, start_num), 
                                      outname);
                    msg ("temporary files renamed");
                } else {
                    // filter out minbgr
                    BigramStream bs (numbered_name (outname, start_num));
                    tempbgr output (outname, nosum, 0, -1, minbgr);
                    while (!bs) {
                        BigramItem bi = *bs;
                        if (bi.value >= minbgr) {
                            bgritem bb (bi.first, bi.value);
                            bb.second = bi.second;
                            output (bb);
                        }
                        ++bs;
                    }
                    string n = numbered_name (outname, start_num);
                    unlink ((n + ".cnt").c_str());
                    unlink ((n + ".idx").c_str());
                    msg ("minbgr filtered out");
                }
            }
        }
    } catch (exception &e) {
        msg (e.what());
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
