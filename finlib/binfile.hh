// Copyright (c) 1999-2019  Pavel Rychly, Milos Jakubicek

#ifndef BINFILE_HH
#define BINFILE_HH

#include <config.hh>
#include "excep.hh"
#include "seek.hh"
#include <string>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#  define MapNetIntFile_ntohl ntohl
#elif defined USE_BYTESWAP
#include <byteswap.h>
#define MapNetIntFile_ntohl bswap_32
#else
#define MapNetIntFile_ntohl(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |  \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

#include <sys/mman.h>


// -------------------- BinFile<AtomType> --------------------

template <class AtomType>
class BinFile {
    FILE *file;
    const std::string name;
    off_t size_s;
public:
    static const int buffsize = 0;
    class const_iterator:
            public std::iterator<std::random_access_iterator_tag, AtomType> {
        FILE *file;
        off_t curr_offset;
        const char *name;
    public:
        typedef AtomType value_type;
        const_iterator (FILE *f, off_t offset, const char *filename)
            : file (f), curr_offset (offset), name (filename) {}
        const_iterator& operator ++() {curr_offset++; return *this;}
        const_iterator& operator --() {curr_offset--; return *this;}
        AtomType operator *() const;
        bool operator== (const const_iterator &x) const
            {return file == x.file && curr_offset == x.curr_offset;}
        bool operator< (const const_iterator &x) const
            {return curr_offset < x.curr_offset;}
        const_iterator& operator +=(off_t delta) 
            {curr_offset += delta; return *this;}
        const_iterator operator +(off_t delta) const
            {return const_iterator (file, curr_offset + delta, name);}
        off_t operator -(const const_iterator &x) const
            {return curr_offset - x.curr_offset;}
    };
    BinFile (const std::string &filename) 
        :file (fopen (filename.c_str(), "rb")), name (filename) {
        if (file == NULL) 
            throw FileAccessError (filename, "BinFile: fopen");
        struct stat statbuf;
        stat (name.c_str(), &statbuf);
        size_s = statbuf.st_size / sizeof (AtomType);
        if (statbuf.st_size % sizeof (AtomType))
            size_s++;
    }
    ~BinFile () {if (file) fclose (file);}
    off_t size () const {return size_s;}
    AtomType operator[] (off_t pos) const;
    const_iterator at (off_t pos) const {
        return const_iterator (file, pos, name.c_str());
    }
};


template <class AtomType>
AtomType BinFile<AtomType>::const_iterator::operator *() const
{
    AtomType i;
    fseeko (file, curr_offset * sizeof (AtomType), SEEK_SET);
    if (fread (&i, 1, sizeof (AtomType), file) < 1)
        throw FileAccessError (name, "BinFile: operator *()");
    return i;
}

template <class AtomType>
AtomType BinFile<AtomType>::operator[] (off_t pos) const
{
    AtomType i;
    fseeko (file, pos * sizeof (AtomType), SEEK_SET);
    if (fread (&i, 1, sizeof (AtomType), file) < 1)
        throw FileAccessError (name, "BinFile: operator []");
    return i;
}


// -------------------- BinCachedFile<AtomType> --------------------

/*
 *                curr // retrieved by *-operator
 *                |              curr_offset
 *                |<--- rest --->| // rest = curr_offset - curr
 *                v              |
 *          [---------buff-------] // 0 <= sizeof(buffer) <= maxbuffsize
 *                               |
 *                               | const_iterator
 *                               v
 * [============= FILE * ========================]
 */
template <class AtomType, int maxbuffsize=128>
class BinCachedFile {
public:
    static const int buffsize = maxbuffsize;
    class const_iterator;
private:
    FILE *file;
    off_t size_s;
    const_iterator *cache;
    off_t last_pos;
    const std::string name;
public:
    class const_iterator:
            public std::iterator<std::random_access_iterator_tag, AtomType> {
        FILE *file;
        AtomType buff[maxbuffsize];
        int buffsize;
        AtomType *curr;
        int rest;
        off_t curr_offset; // from which offset to read for next buffer
        const std::string name;
    public:
        typedef AtomType value_type;
        const_iterator (FILE *f, off_t offset, const std::string filename,
                        int bsize)
            : file (f), buffsize (bsize), rest (0), curr_offset (offset),
              name (filename)
            {++*this;}
        const_iterator (const const_iterator *cache, off_t offset) 
            : file (cache->file), buffsize (cache->buffsize),
              rest (cache->rest), curr_offset (cache->curr_offset),
              name (cache->name) {
            if (offset >= curr_offset - buffsize && offset < curr_offset) {
                memcpy (buff, cache->buff, buffsize * sizeof(AtomType));
                rest =  curr_offset - offset;
                curr = buff + (buffsize - rest);
            } else {
                rest = 0; 
                curr_offset = offset;
                ++*this;
            }
        }
        const_iterator (const const_iterator &x)
            : file (x.file), buffsize (x.buffsize),
              curr (buff + (x.curr - x.buff)),
              rest (x.rest), curr_offset (x.curr_offset), name(x.name) {
            memcpy (buff, x.buff, buffsize * sizeof(AtomType));
        }
        const_iterator& operator=(const const_iterator &x) {
            file = x.file; buffsize = x.buffsize;
            curr = buff + (x.curr - x.buff);
            rest = x.rest; curr_offset = x.curr_offset;
            memcpy (buff, x.buff, buffsize * sizeof(AtomType));
            return *this;
        }
        const_iterator& operator ++() {
            if (rest <= 1) {
                if (fseeko (file, curr_offset * sizeof(AtomType), SEEK_SET))
                    throw FileAccessError (name, "BinCachedFile++");
                size_t rest_bytes = fread (buff, 1,
                                        sizeof (AtomType) * maxbuffsize, file);
                rest = rest_bytes / sizeof (AtomType);
                if (rest_bytes % sizeof (AtomType))
                    rest++;
                buffsize = rest;
                curr = buff;
                curr_offset += rest;
            } else {
                curr++;
                rest--;
            }
            return *this;
        }
        const_iterator& operator --() {(*this)+= -1; return *this;}
        AtomType operator *() const {
            if (!rest)
                throw FileAccessError (name, "BinCachedFile*");
            return *curr;
        }
        const_iterator& operator +=(off_t delta) {
            if ((delta >= 0 && delta < rest) 
                || (delta < 0 && delta >= buff - curr)) {
                rest -= delta;
                curr += delta;
            } else {
                off_t new_offset = curr_offset + delta - rest;
                if (new_offset < 0)
                    return *this;
                curr_offset = new_offset;
                rest = 0;
                ++*this;
            }
            return *this;
        }
        const_iterator operator +(off_t delta) 
            {return const_iterator (this, curr_offset - rest + delta);}
        off_t operator -(const const_iterator &x) const
            {return curr_offset - rest - x.curr_offset + x.rest;}
        bool operator< (const const_iterator &x) const
            {return curr_offset - rest < x.curr_offset - x.rest;}
        off_t tell() {return curr_offset - rest;}
    };
    BinCachedFile (const std::string &filename) 
        :file (fopen (filename.c_str(), "rb")), cache(NULL), name (filename) {
        if (file == NULL) 
            throw FileAccessError (filename, "BinCachedFile: fopen");
        struct stat statbuf;
        stat (name.c_str(), &statbuf);
        size_s = statbuf.st_size / sizeof (AtomType);
        if (statbuf.st_size % sizeof (AtomType))
            size_s++;
    }
    ~BinCachedFile () {
        if (file) 
            fclose (file); 
        if (cache) 
            delete cache;
    }
    const_iterator at (off_t pos) const {
        if (cache) return const_iterator (cache, pos);
        return const_iterator (file, pos, name, maxbuffsize);
    }
    AtomType operator[] (off_t pos) {
        if (!cache) {
            cache = new const_iterator (file, pos, name, maxbuffsize);
            last_pos = pos;
        } else if (last_pos != pos) {
            (*cache) += (pos - last_pos);
            last_pos = pos;
        }
        return **cache;
    }
    off_t size () const {return size_s;}
};

// -------------------- MapBinFile<AtomType> --------------------

template <class AtomType>
class MapBinFile {
public:
    typedef AtomType value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    static const int buffsize = 0;
    const std::string name;
    AtomType *mem;
    AtomType *base;
    off_t size_s;
    off_t file_size;
protected:
    bool nomap;
public:
    MapBinFile (const std::string &filename);
    ~MapBinFile () {
        if (nomap) delete[] mem;
        else munmap ((void *)mem, file_size);
    }

    iterator at (off_t pos) {return base + pos;}
    const_iterator at (off_t pos) const {return base + pos;}
    off_t size () const {return size_s;}
    AtomType operator[] (off_t pos) const {return base [pos];}
    void skip_header (unsigned int items) {
        if (items < size_s) {
            base += items;
            size_s -= items;
        }
    }
};

#ifndef MIN_MAP_SIZE
#define MIN_MAP_SIZE 7000
#endif

template <class AtomType>
MapBinFile<AtomType>::MapBinFile (const std::string &filename) :
    name (filename)
{
    int fd = open (filename.c_str(), O_RDONLY);
    if (fd < 0)
        throw FileAccessError (filename, "MapBinFile:open");
    file_size = lseek (fd, 0, SEEK_END);
    if (file_size < 0) {
        close (fd);
        throw FileAccessError (filename, "MapBinFile:open");
    }

    size_s = (file_size + sizeof (AtomType) - 1) / sizeof (AtomType);
    nomap = file_size < MIN_MAP_SIZE;

    if (nomap) {
        mem = new AtomType [size_s];
        ssize_t read_size = pread (fd, mem, file_size, 0);
        if (read_size < 0 || read_size < file_size) {
            delete[] mem;
            close (fd);
            throw FileAccessError (filename, "MapBinFile:fread");
        }
    } else {
        mem = (AtomType *) mmap (0, file_size, PROT_READ, MAP_SHARED, fd, 0);
        if (mem == MAP_FAILED) {
            close (fd);
            throw FileAccessError (filename, "MapBinFile:mmap");
        }
    }
    close (fd);
    base = mem;
}


// -------------------- MapNetIntFile --------------------

class MapNetIntFile: public MapBinFile<int> 
{
public:
    class const_iterator {
        int *p;
    public:
        typedef int value_type;
        const_iterator (int *init) : p(init) {}
        const_iterator& operator ++() {++p; return *this;}
        int operator *() const {return MapNetIntFile_ntohl (*p);}
        const_iterator& operator +=(int delta) {p += delta; return *this;}
        const_iterator operator +(int delta) const {return p + delta;}
        int next() {int r = MapNetIntFile_ntohl (*p); ++p; return r;}
    };
    MapNetIntFile (const std::string &filename)
        : MapBinFile<int> (filename) {}
    const_iterator at (int pos) const {return base + pos;}
    int operator[] (int pos) const {return MapNetIntFile_ntohl (base [pos]);}
};


#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
