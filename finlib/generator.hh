// Copyright (c) 1999-2020  Pavel Rychly

#ifndef GENERATOR_HH
#define GENERATOR_HH

#include <vector>
#include <string>

using namespace std;

template <class Value>
class Generator {
protected:
    Generator() {}
public:
    Value operator()() {return next();}
    virtual Value next() = 0;
    virtual bool end() = 0;
    virtual const int size() const {return 1;}
    virtual ~Generator() {}
};


template <class Value>
class EmptyGenerator: public Generator<Value> {
protected:
    const Value val; 
public:
    EmptyGenerator (Value v=Value()): val(v) {}
    virtual Value next() {return val;}
    virtual bool end() {return true;}
    virtual const int size() const {return 0;}
    virtual ~EmptyGenerator() {}
};


template <class Value>
class SequenceGenerator: public Generator<Value> {
protected:
    Value curr; 
    const Value last;
    const int count;
public:
    SequenceGenerator (Value first, Value last): curr(first), last(last), count(last - curr +1) {}
    virtual Value next() {return curr++;}
    virtual bool end() {return curr > last;}
    virtual const int size() const {return count;}
    virtual ~SequenceGenerator() {}
};

template <class Value>
class ArrayGenerator: public Generator<Value> {
protected:
    Value *array, *curr, *last;
    const int count;
public:
    ArrayGenerator (Value *a, unsigned size):
        array (a), curr (a), last (a + size), count (size) {}
    virtual Value next() {return *curr++;}
    virtual bool end() {return curr >= last;}
    virtual const int size() const {return count;}
    virtual ~ArrayGenerator() {delete[] array;}
};

class IdStrGenerator {
    Generator <int> *intGen = NULL;
    Generator <string> *strGen = NULL;
    int currId;
    string currStr;
    bool finished;
public:
    IdStrGenerator () {}
    IdStrGenerator (Generator<int> *igen, Generator<string> *sgen):
        intGen(igen), strGen(sgen), finished(false) { next(); }
    virtual void next() {
        if (intGen->end()) {
            finished = true;
            return;
        }
        currId = intGen->next();
        currStr = strGen->next();
    }
    virtual int getId() {return currId;}
    virtual string getStr() {return currStr;}
    virtual bool end() {return finished;}
    virtual const int size() const {return intGen->size();}
    virtual ~IdStrGenerator() {delete intGen; delete strGen;}
};

class EmptyIdStrGenerator: public IdStrGenerator {
public:
    EmptyIdStrGenerator () {}
    virtual void next() {}
    virtual int getId() {return 0;}
    virtual string getStr() {return "";}
    virtual bool end() {return true;}
    virtual const int size() const {return 0;}
};

class SingleIdStrGenerator: public IdStrGenerator {
protected:
    int id;
    string str;
    bool finished;
public:
    SingleIdStrGenerator (int i, const char *s): id(i), str(s), finished(false) {}
    virtual void next() {finished = true;}
    virtual int getId() {return id;}
    virtual string getStr() {return str;}
    virtual bool end() {return finished;}
    virtual const int size() const {return 1;}
    virtual ~SingleIdStrGenerator() {}
};

class ArrayIdStrGenerator: public IdStrGenerator {
protected:
    vector<string> *strs;
    int* ids;
    bool finished;
    size_t curr;
public:
    ArrayIdStrGenerator (vector<string> *v, int *i): strs(v), ids(i), finished(false), curr(0) {}
    virtual void next() {finished = (++curr >= strs->size());}
    virtual int getId() {return ids[curr];}
    virtual string getStr() {return (*strs)[curr];}
    virtual bool end() {return finished;}
    virtual const int size() const {return strs->size();}
    virtual ~ArrayIdStrGenerator() {}
};

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
