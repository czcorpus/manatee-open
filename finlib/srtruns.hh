// Copyright (c) 1999-2010 Pavel Rychly

#ifndef SRTRUNS_HH
#define SRTRUNS_HH

#include "excep.hh"
#include <algorithm>


// -------------------- make_sorted_runs --------------------
template <class RandomAccessIterator, class Distance, class T>
void add_to_heap (RandomAccessIterator first, Distance holeIndex,
                  Distance len, T value)
{
    Distance child = 2 * holeIndex + 2;
    while (child <= len) {
        if ((child == len) || (*(first + (child - 1)) < *(first + child)))
            child--;
        if (value < *(first + child)) 
            break;
        *(first + holeIndex) = *(first + child);
        holeIndex = child;
        child = 2 * child + 2;
    }
    *(first + holeIndex) = value;
};



template <class InputIterator, class F, class T>
void make_sorted_runs_aux (InputIterator in, F out, const int buf_size, T*) 
{
    T *buf = new T[buf_size];
    if (!buf)
        throw MemoryAllocationError ("make_sorted_runs");
    int insert, next = buf_size;

    for (insert = buf_size -1; !in && insert >= 0; ++in, --insert) {
        add_to_heap (buf, insert, buf_size, *in);
    }
    while (!in) {
        out (*buf);
        if (*buf == *in || *buf < *in)
            add_to_heap (buf, 0, next, *in);
        else {
            if (--next == 0) {
                // next run
                next = buf_size;
                add_to_heap (buf, 0, buf_size, *in);
            } else {
                add_to_heap (buf, 0, next, buf[next]);
                add_to_heap (buf, next, buf_size, *in);
            }
        }
        ++in;
    }
    std::sort (buf + insert +1, buf + buf_size);
    //for_each<T*, F&> (buf + insert +1, buf + buf_size, out);
    std::for_each (buf + insert +1, buf + buf_size, out);
    delete[] buf;
}

template <class InputIterator, class F>
inline void make_sorted_runs (InputIterator in, F out, const int buf_size) 
{
    make_sorted_runs_aux<InputIterator, F, 
        typename std::iterator_traits<InputIterator>::value_type> 
        (in, out, buf_size, 0);
}

template <class TmpFile, class T>
class SortedRuns
{
    int buf_size;
    T *buf;
    int insert, next;
    TmpFile *out;
private:
    SortedRuns (SortedRuns &x) {}
public:
    SortedRuns (TmpFile *tf, int bs)
        : buf_size (bs), buf (new T [buf_size]), 
          insert (buf_size -1), next (buf_size), out (tf)
    {}
    ~SortedRuns() {
        flush();
        delete[] buf;
        delete out;
    }
    void flush() {
        if (next != buf_size)
            out->nextRun();
        std::sort (buf + insert +1, buf + buf_size);
        for (T *i=buf + insert +1; i < buf + buf_size; i++)
            (*out)(*i);
        insert = buf_size -1;
        next = buf_size;
    }
    void operator() (T x) {this->put (x);}
    void put (T x) {
        if (insert >= 0) {
            // filling phase
            add_to_heap (buf, insert--, buf_size, x);
        } else {
            // sorting phase
            (*out) (*buf);
            if (*buf == x || *buf < x)
                add_to_heap (buf, 0, next, x);
            else {
                if (--next == 0) {
                    out->nextRun();
                    next = buf_size;
                    add_to_heap (buf, 0, buf_size, x);
                } else {
                    add_to_heap (buf, 0, next, buf[next]);
                    add_to_heap (buf, next, buf_size, x);
                }
            }
        }
    }
};


template <class T>
class StoreTopItems
{
    int buf_size;
    T *buf;
    int insert, next;
private:
    StoreTopItems (StoreTopItems &x) {}
public:
    StoreTopItems (int maxitems, T *storage)
        : buf_size (maxitems), buf (storage), 
          insert (buf_size -1), next (buf_size)
    {}
    void operator() (T x) {this->put (x);}
    void put (T x) {
        if (insert >= 0) {
            // filling phase
            add_to_heap (buf, insert--, buf_size, x);
        } else {
            // sorting phase
            if (*buf < x)
                add_to_heap (buf, 0, next, x);
        }
    }
    void clear() {
        insert = buf_size -1;
        next = buf_size;
    }
    T *begin() {
        return buf + insert +1;
    }
    T *end() {
        return buf + buf_size;
    }
};


template <class Output, class T>
class TopItems
{
    T *buf;
    StoreTopItems<T> storage;
    Output &out;
private:
    TopItems (TopItems &x) {}
public:
    TopItems (int maxitems, Output &o)
        : buf (new T [maxitems]), storage (maxitems, buf), out (o)
    {}
    ~TopItems() {
        flush();
        delete[] buf;
    }
    void operator() (T x) {storage.put(x);}
    void put (T x) {storage.put(x);}
    void flush() {
        std::sort (storage.begin(), storage.end());
        for (T* i = storage.end(); i != storage.begin();)
            out (*--i);
        storage.clear();
    }
};

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
