// Copyright (c) 2011-2016  Pavel Rychly, Milos Jakubicek

#include "fromtof.hh"
#include "bitio.hh"
#include "log.hh"
#include "config.hh"
#include "levels.hh"
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

typedef uint8_t wmap_atom_type;


class OutFileBits32 {
protected:
    FILE *file;
    wmap_atom_type curr;
    off_t filetell;
public:
    OutFileBits32 (const char *name) :file (fopen (name, "wb")), filetell(32) {
        fwrite ("\243finALN\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 1, 8, file);
        seek (32);
    }
    ~OutFileBits32 () {fclose (file);}
    wmap_atom_type& operator* () {return curr;}
    OutFileBits32& operator++() {
        fwrite (&curr, 1, 1, file);
        filetell++;
        return *this;
    }
    off_t tell() {return filetell;}
    void seek(off_t s) {filetell=s; fseek (file, s*sizeof(wmap_atom_type), SEEK_SET);}
};


class ChangeWriter {
typedef write_bits<OutFileBits32&,wmap_atom_type, OutFileBits32&, Position> Writer;
protected:
    OutFileBits32 *outbits;
    Writer *writer;
public:
    ChangeWriter(const char *name)
        : outbits(new OutFileBits32(name)), writer(new Writer(*outbits)) {
        //outbits->seek(32);
    }
    ~ChangeWriter() {delete writer; delete outbits;}
    void gamma (Position x) {writer->gamma(x);}
    void delta (Position x) {writer->delta(x);}
    off_t tell() {
        return outbits->tell()*8 + writer->bitslen();
    }
};


// strip trailing white spaces
void stripend (string &str)
{
    size_t i = str.length();
    while (i > 0 && (str[i-1] == ' ' || str[i-1] == '\t' || str[i-1] == '\r'))
        i--;
    if (i < str.length())
        str.erase (i);
}

bool splitpair (const string &str, int &first, int &diff)
{
    size_t c = str.find(',');
    if (c == string::npos) {
        c = str.find(':');
        if (c == string::npos) {
            first = atol(str.c_str());
            if (first == -1)
                diff = 0;
            else
                diff = 1;
        } else {
            first = atol(str.substr(0,c).c_str());
            diff = atol(str.substr(c+1).c_str()) - first+1;
            return true;
        }
    } else {
        first = atol(str.substr(0,c).c_str());
        diff = atol(str.substr(c+1).c_str()) - first+1;
    }
    return false;
}


int processfile (istream &in_file, const string &outfilename,
                  int segsize=512)
{
    ToFile<int64_t> alidxf (outfilename + ".idx");
    ChangeWriter alchnf (outfilename.c_str());

    string line;
    int currorg = 0;
    int currnew = 0;
    int sizeof1to1 = 0;
    int numofchng = 0;
    unsigned long long int linenum = 0;
    int ret = 0;

    while (getline (in_file, line)) {
        linenum++;
        stripend (line);
        if (line.length() == 0 || line[0] == '#' 
            || line == "{{{" || line == "}}}")
            continue;
        size_t tab = line.find('\t');
        if (tab == string::npos) {
            CERR << linenum << ": no TAB separated pair" << endl;
            ret = 1;
            continue;
        }
        int fi,fid, se,sed;
        bool one_seq1 = splitpair (line.substr(0,tab), fi, fid);
        bool one_seq2 = splitpair (line.substr(tab+1), se, sed);
        if (one_seq1 != one_seq2) {
            CERR << linenum << ": inconsistent 1:1 range mapping: " << line << endl;
            return 1;
        }
        if ((fid > 0 && fi != currorg) || fid < 0
            || (sed > 0 && se != currnew) || sed < 0) {
            CERR << linenum << ": not in the sequence: " << line << endl;
            return 1;
        }

        if (numofchng <= 0) {
            // close segment
            alidxf.put(currorg);
            alidxf.put(currnew);
            alidxf.put(alchnf.tell());
            numofchng = segsize;
        }

        if (sizeof1to1 > 0) {
            // inside 1-1 region
            if (one_seq1) {
                // continue 1-1 region with range
                currorg += fid; currnew += fid;
                sizeof1to1 += fid;
                continue;
            } else if (fid == 1 && sed == 1) {
                // continue 1-1 region
                currorg++; currnew++;
                sizeof1to1++;
                continue;
            } else {
                // closing 1-1 region
                alchnf.gamma(MLTStream::KEEP);
                alchnf.delta(sizeof1to1);
                sizeof1to1 = 0;
                numofchng--;
            }
        }

        if (fid == 0) {
            // 0-n
            alchnf.gamma(MLTStream::INSERT);
            alchnf.delta(sed);
        } else if (one_seq1) {
            // 1-1 range
            sizeof1to1 = fid;
            numofchng++;
        } else if (fid == 1 && sed == 1) {
            // 1-1
            sizeof1to1 = 1;
            numofchng++;
        } else if (sed == 0) {
            // n-0
            alchnf.gamma(MLTStream::DELETE);
            alchnf.delta(fid);
        } else {
            // 1-n, m-n
            alchnf.gamma(MLTStream::MORPH);
            alchnf.delta(fid);
            alchnf.gamma(sed);
        }
        if (one_seq1) {
            currorg += fid;
            currnew += fid;
        } else {
            currorg += fid;
            currnew += sed;
        }
        numofchng--;
    }

    if (sizeof1to1 > 0) {
        // closing 1-1 region
        alchnf.gamma(MLTStream::KEEP);
        alchnf.delta(sizeof1to1);
    }
                
    alidxf.put(currorg);
    alidxf.put(currnew);
    alidxf.put(alchnf.tell());
    return ret;
}


int main (int argc, const char **argv)
{
    if (argc != 3) {
        if (argc > 1) {
            const char *o = argv[1];
            if (*(o++) == '-' && *o == 'v') {
                cout << VERSION_STR << '\n';
                return 2;
            }
        }

        cerr << "usage: mkalign INPUTFILE OUTPUTFILE\n";
        return 1;
    }
    try {
        const char *filename = argv[1];
        if (filename[0] == '-' && filename[1] == '\0')
            processfile (cin, argv[2]);
        else {
            ifstream in_file (filename);
            if (!in_file.is_open()) {
                CERR << "mkalign: Error opening file: " << filename << '\n'; 
                return 1;
            }
            return processfile (in_file, argv[2]);
        }
    } catch (exception &e) {
        CERR << "mkalign error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}


// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
