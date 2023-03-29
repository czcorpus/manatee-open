//  Copyright (c) 2003-2008  Pavel Rychly

#include "concord.hh"
#include "bgrstat.hh"
#include <algorithm>
#include <utility>
#include <sstream>

using namespace std;

void Concordance::set_linegroup (ConcIndex linenum, int group)
{
    if (linenum < 0 || linenum >= size())
	return;
    if (!linegroup) {
	linegroup = new linegroup_t(size(), 0);
    }
    if (view)
	linenum = (*view)[linenum];
    (*linegroup)[linenum] = group;
}

void Concordance::set_linegroup_globally (int group)
{
    if (!linegroup) {
	linegroup = new linegroup_t(size(), group);
    } else {
	for (linegroup_t::iterator i = linegroup->begin(); 
	     i != linegroup->end(); ++i)
	    *i = group;
    }
}

int Concordance::set_linegroup_at_pos (Position pos, int group)
{
    int orggroup = 0;
    if (pos < 0 || pos >= corp->size())
	return orggroup;
    if (!linegroup) {
	linegroup = new linegroup_t(size(), 0);
    }
    ConcIndex idx = 0;
    while (beg_at (idx) < pos && idx < size())
	idx++;
    if (beg_at (idx) == pos) {
	orggroup = (*linegroup)[idx];
	(*linegroup)[idx] = group;
    }
    return orggroup;
}

void Concordance::set_linegroup_from_conc (Concordance *master)
{
    if (!master->linegroup) return;
    if (!linegroup) {
	linegroup = new linegroup_t(size(), 0);
    }
    ConcIndex thisidx = 0;
    ConcIndex masteridx = 0;
    while (thisidx < size() && masteridx < master->size()) {
	if (beg_at (thisidx) == master->beg_at (masteridx))
	    (*linegroup)[thisidx++] = (*master->linegroup)[masteridx++];
	else if (beg_at (thisidx) < master->beg_at (masteridx))
	    thisidx++;
	else
	    masteridx++;
    }
}

void Concordance::get_linegroup_stat (std::map<short int,ConcIndex> &lgs)
{
    lgs.clear();
    if (!linegroup) return;
    for (linegroup_t::iterator i = linegroup->begin(); 
	 i != linegroup->end(); ++i)
	lgs[*i]++;
}

int Concordance::get_new_linegroup_id()
{
    if (!linegroup) return 1;
    short int maxid = 0;
    for (linegroup_t::iterator i = linegroup->begin(); 
	 i != linegroup->end(); ++i)
	if (*i > maxid)
	    maxid = *i;
    return maxid +1;
}


void Concordance::delete_linegroups (const char *grps, bool invert)
{
    if (!linegroup) return;
    std::map<short int,bool> deletegrp;
    short int g;
    istringstream in (grps);
    while (in >> g)
	deletegrp[g] = true;

    ConcIndex newsize = 0;
    for (linegroup_t::iterator i = linegroup->begin();
	 i != linegroup->end(); ++i)
	if ((invert && deletegrp[*i]) || (!invert && !deletegrp[*i]))
	    newsize++;
    if (newsize == size())
	return;
    if (view) {
	delete view;
	view = NULL;
    }

    linegroup_t *newlineg = new linegroup_t(newsize);
    linegroup_t::iterator lgnew = newlineg->begin();
    ConcItem *newrng = (ConcItem *) malloc (newsize * sizeof (ConcItem));
    ConcItem *rinew = newrng;
    ConcItem *riold = rng;
    for (linegroup_t::iterator i = linegroup->begin();
	 i != linegroup->end(); ++i, ++riold)
	if ((invert && deletegrp[*i]) || (!invert && !deletegrp[*i])) {
	    *(rinew++) = *riold;
	    *(lgnew++) = *i;
	}

    used = newsize;
    allocated = newsize;
    riold = rng;
    rng = newrng;
    delete riold;
    delete linegroup;
    linegroup = newlineg;
}

#define CLUST 0
#if CLUST
int static locateclust (vector<int> &cl, int id)
{
    vector<int> path;
    int n;
    while ((n = cl[id]) > 0) {
	path.push_back (id);
	id = n;
    }
    if (path.size() > 1) {
	for (vector<int>::const_iterator i = path.begin(); 
	     i != path.end(); i++)
	    cl[*i] = id;
    }
    return id;
}
#endif

void Concordance::make_grouping ()
{
#if CLUST
    // XXX update int -> Position !!!
    PosAttr *a = corp->get_attr ("-");
    int fromw = -5, tow = 5, minfreq = 5, minbgr = 3;

    // count collocations
    map<int,int> bs;
    for (int i = 0; i < size(); i++) {
	if (rng[i].beg == -1) continue;
	if (fromw < 0) {
	    IDIterator *pi = a->posat (rng[i].beg + fromw);
	    for (int d = fromw; d < 0 && d <= tow; d++) {
		int id = pi->next();
		if (a->freq (id) >= minfreq)
		    bs [id] ++;
	    }
	    delete pi;
	}
	if (tow > 0) {
	    int d = fromw > 0 ? fromw : 1;
	    IDIterator *pi = a->posat (rng[i].end -1 + d);
	    for (; d <= tow; d++) {
		int id = pi->next();
		if (a->freq (id) >= minfreq)
		    bs [id] ++;
	    }
	    delete pi;
	}
    }

    // make scorese (silence)
    typedef vector<pair<double,int> > resvect;
    resvect results;
    double N = corp_size;
    double f_B = viewsize();
    for (map<int,int>::const_iterator i = bs.begin(); i != bs.end(); ++i) {
	if ((*i).second < minbgr) continue;
	int freq = a->freq ((*i).first);
	results.push_back (make_pair (bgr_silence ((*i).second, freq, f_B, N), 
				     (*i).first));
    }
    // XXX use partial_sort (see concstat.cc)
    ::sort (results.begin(), results.end());

    // prepare id2weight
    int toskip = results.size() / 5;
    if (results.size() > 100) 
	toskip = results.size() - 20;
    map<int,double> id2weight;
    for (resvect::const_iterator i = results.begin() + toskip; 
	 i != results.end(); ++i) {
	id2weight[(*i).second] = (*i).first;
    }
    
    // prepare id lists
    typedef map<int, vector<int> > toproct;
    toproct toproc;
    for (int i = 0; i < size(); i++) {
	if (rng[i].beg == -1) continue;
	if (fromw < 0) {
	    IDIterator *pi = a->posat (rng[i].beg + fromw);
	    for (int d = fromw; d < 0 && d <= tow; d++) {
		int id = pi->next();
		if (id2weight.find (id) != id2weight.end())
		    toproc [id].push_back(i);
	    }
	    delete pi;
	}
	if (tow > 0) {
	    int d = fromw > 0 ? fromw : 1;
	    IDIterator *pi = a->posat (rng[i].end -1 + d);
	    for (; d <= tow; d++) {
		int id = pi->next();
		if (id2weight.find (id) != id2weight.end())
		    toproc [id].push_back(i);
	    }
	    delete pi;
	}
    }

    //make main matrix
    typedef map<pair<int,int>, double> MPD;
    MPD w;
    for (toproct::const_iterator i = toproc.begin(); i != toproc.end(); ++i) {
	const vector<int> &v = (*i).second;
	for (vector<int>::const_iterator j=v.begin(); j != v.end(); ++j) {
	    for (vector<int>::const_iterator k=j+1; k != v.end(); ++k) {
		w[make_pair(*j, *k)] += (*i).first;
	    }
	}
    }
    
    //make sort bgr
    typedef vector<pair<double, pair<int,int> > > VPP;
    VPP bgr;
    for (MPD::const_iterator i = w.begin(); i != w.end(); ++i) {
	bgr.push_back (make_pair ((*i).second, (*i).first));
    }
    ::sort (bgr.begin(), bgr.end());
    

    //make clusters
    vector<int> cl (size(), -1);
    VPP::const_iterator p1, p2;
    
    // phase 1: first 30 %
    //    free clustering (select median level delta MD)
    p1 = bgr.begin();
    p2 = bgr.begin() + bgr.size() * 3 / 10;
    for (; p1 != p2; ++p1) {
	int c1 = locateclust (cl, (*p1).second.first);
	int c2 = locateclust (cl, (*p1).second.second);
	if (c1 != c2) {
	    cl [c2] = c1;
	    cl [c1] = - (int)((*p1).first * 1000);
	}
    }


    // phase 2: next 20 % (e.i. up to 50 %)
    //    add singletons + join groups on level < 1.3 * MD + group level

    // phase 3: rest
    //    add singletons on level < 2 * MD + group level

    

    // init linegroup vector
    if (!linegroup) {
	linegroup = new linegroup_t(size(), 0);
    } else {
	//set 0
    }

#endif
}
