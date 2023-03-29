//  Copyright (c) 2002-2020  Pavel Rychly, Milos Jakubicek

#include "posattr.hh"

template <class RevClass>
class IDPosIteratorFromRevs : public IDPosIterator {
    class FastStreamWithID : public FastStream {
        FastStream *src;
        int id;
    public:
        FastStreamWithID (FastStream *s, int i) : src (s), id (i) {}
        ~FastStreamWithID() {delete src;}
        void add_labels (Labels &lab) {return src->add_labels (lab);}
        Position peek() {return src->peek();}
        Position next() {return src->next();}
        Position find (Position pos) {return src->find (pos);}
        NumOfPos rest_min() {return src->rest_min();}
        NumOfPos rest_max() {return src->rest_max();}
        Position final() {return src->final();}
        int get_id() {return id;}
    };
    class QOrVNodeWithID : public QOrVNode {
    public:
        QOrVNodeWithID (vector<FastStream*> *source): QOrVNode (source, false){}
        int peek_id() {return ((FastStreamWithID*) src[0].second)->get_id();}
    };
    QOrVNodeWithID *fsnode;
public:
    IDPosIteratorFromRevs (RevClass *rev, Position pos) {
        int maxid = rev->id_range();
        vector<FastStream*> *fsv = new vector<FastStream*> (maxid);
        for (int id = 0; id < maxid; id++)
            (*fsv)[id] = new FastStreamWithID (rev->id2poss(id), id);
        fsnode = new QOrVNodeWithID (fsv);
        fsnode->find (pos);
    }
    ~IDPosIteratorFromRevs() {delete fsnode;}
    void next() {fsnode->next();}
    Position peek_pos() {return fsnode->peek();}
    int peek_id() {return fsnode->peek_id();}
    bool end() {return fsnode->peek() >= fsnode->final();}
};

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
