// Copyright (c) 1999-2002  Pavel Rychly

#ifndef REFCOUNT_HH
#define REFCOUNT_HH

// viz /home/pary/proj/cqm/tools/src/cqs-base.h

template<class C>
class RefCount {
private:
    class RefCounted {
    public:
	C *ptr;
	RefCounted (C *p): ptr (p), count (1) {}
	int count;
	void take() { count++; }
	void drop() { if (!--count) {delete ptr; delete this;} }
    };
    RefCounted *ref;
public:
    RefCount (C *p) { if (p) ref = new RefCounted (p); }
    RefCount (const RefCount &c) : ref (c.ref) { if (ref) ref->take(); }
    RefCount& operator= (const RefCount &c) {
	if (ref) ref->drop();
	ref = c.ref;
	if (ref) ref->take();
	return *this;
    }

    ~RefCount() { if (ref) ref->drop(); }
    C& operator*() { return *(ref->ptr); }
    C* operator->() { return ref->ptr; }
    const C& operator*() const { return *(ref->ptr); }
    const C* operator->() const { return ref->ptr; }
    operator bool() { return (ref!=0); }
};


template<class Iterator>
class CRefIter {
protected:
    RefCount<Iterator> iter;
public:
    CRefIter (Iterator *i) : iter (i) {}
    typename Iterator::value_type& operator*() {return **iter;}
    CRefIter& operator++ () {++*iter; return *this;}
};
 
    
    


#endif
