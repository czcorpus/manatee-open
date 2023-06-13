// Copyright (c) 2021 Ondrej Herman
#include <inttypes.h>
#include <iostream>
#include "corpus.hh"
#include "subcorp.hh"
#include "fingetopt.hh"

void usage(char **argv) {
    std::cerr << "usage: " << argv[0] << " CORPUS [STRUCTURE [ATTRIBUTE]] [-o BASE] [-s SUBCPATH]" << std::endl
        << "count corpus positions covered by structure attribute values" << std::endl;
}

std::ostream& log() {
    const time_t now = time(0);
    char buffer[32];
    strftime(buffer, 32, "[%Y%m%d-%H:%M:%S]", localtime(&now));
    std::cerr << buffer << " mktokencov: ";
    return std::cerr;
}

inline void ensuresize (char *&ptr, int &allocsize, int needed,
                                const char *where)
{
    if (allocsize >= needed) return;
    char *p = (char *)realloc (ptr, needed);
    if (!p)
        throw MemoryAllocationError (where);
    allocsize = needed;
    ptr = p;
}

void update_multivalue(PosAttr *attr, std::vector<uint64_t> &tokencov, const char *multisep) {
    for(int oid = 0; oid < attr->id_range(); oid++) {
        const char *full_value = attr->id2str(oid);
        if(!multisep || *multisep == 0) {  // zero-length MULTISEP, attribute value is split into bytes
            if(strlen(full_value) <= 1)
		continue;
            for(size_t i = 0; i < strlen(full_value) - 1; i++) {
                const char new_val[] = {full_value[i], 0};
                int nid = attr->str2id(new_val);
                tokencov[nid] += tokencov[oid];
            }
        } else {  // MULTISEP is a single byte delimiter
            if (strchr(full_value, *multisep) == NULL)
                continue;
            static int value_len = 0;
            static char *value = NULL;
            ensuresize (value, value_len, strlen (full_value) +1, "store_multival");
            value[0] = 0;
            strncat (value, full_value, value_len -1);
            for (char *s = strtok (value, multisep); s; s = strtok (NULL, multisep)) {
                int nid = attr->str2id(s);
                tokencov[nid] += tokencov[oid];
            }
        }
    }
}

uint64_t intersect_size(const Position &abeg, const Position &aend,
                const Position &bbeg, const Position &bend) {
    if (bbeg > aend || bend < abeg) return 0;
    else return min(aend, bend) - max(abeg, bbeg);
}

void write_tokenfile(Corpus *corp, SubCorpus *subc, string &structname, vector<string> &attrnames, string &base) {
    if (attrnames.empty()) {
        log() << "skipping token coverage calculation for structure "
              << structname << " without attributes" << endl;
	return;
    }
    Structure *struc = corp->get_struct(structname);
    RangeStream *structure = struc->rng->whole();

    std::vector<ToFile<uint64_t>*> ofs;
    std::vector<IDIterator*> its;
    std::vector<std::vector<uint64_t>> tokencov;
    std::vector<PosAttr*> attrs;
    for (auto attrname: attrnames) {
        string fname = base + structname + "." + attrname + ".token";
        log() << "preparing " << fname << endl;

        ofs.push_back(new ToFile<uint64_t>(fname));
        PosAttr *attr = struc->get_attr(attrname);
        attrs.push_back(attr);
        its.push_back(attr->posat(0));
        tokencov.push_back(vector<uint64_t>(attr->id_range()));
    }

    log() << "calculating token coverage for " << structname << endl;

    if (subc) {
        ranges *subcorp = subc->subcorp;
        uint64_t subc_firstnum = 0, subc_lastnum = 0;

        while (!structure->end()) {
            // extend the active subcorpus area
            while (structure->peek_end() > subcorp->end_at(subc_lastnum) &&
                    subc_lastnum < (uint64_t) subcorp->size())
                subc_lastnum++;

            while (subc_firstnum+1 < (uint64_t) subcorp->size() &&
                    subcorp->beg_at(subc_firstnum+1) <= structure->peek_beg())
                subc_firstnum++;

            size_t len = 0;
            for (uint64_t i = subc_firstnum; i <= subc_lastnum; i++)
                len += intersect_size(
                    subcorp->beg_at(i), subcorp->end_at(i),
                    structure->peek_beg(), structure->peek_end());

            for (size_t i = 0; i < its.size(); i++) {
                uint32_t structattrid = its[i]->next();
                tokencov[i][structattrid] += len;
            }

            structure->next();
        }
    } else {
        while (!structure->end()) {
            uint64_t len = structure->peek_end() - structure->peek_beg();
            for (size_t i = 0; i < its.size(); i++) {
                uint32_t structattrid = its[i]->next();
                tokencov[i][structattrid] += len;
            }
            structure->next();
        }
    }

    delete structure;

    for (size_t i = 0; i < attrs.size(); i++) {
        auto ao = struc->conf->find_attr(attrs[i]->name);
        if (struc->conf->str2bool(ao["MULTIVALUE"])) {
            log() << "updating multivalue coverage for " << attrs[i]->name << endl;
            update_multivalue(attrs[i], tokencov[i], ao["MULTISEP"].c_str());
        }
    }

    log() << "writing token coverage for " << structname << endl;

    for (size_t i = 0; i < attrs.size(); i++) {
        for (uint64_t cov: tokencov[i])
            ofs[i]->put(cov);
        delete ofs[i];
        delete its[i];
    }

    log() << "finished writing token coverage for " << structname << endl;
}


int main (int argc, char **argv) {
    string corpname, structname, attrname, subc, base;

    int c;
    while ((c = getopt (argc, argv, "s:o:h")) != -1) {
        switch(c) {
            case 's':
                subc = optarg;
                break;
            case 'o':
                base = optarg;
                break;
            case 'h':
            default:
                usage(argv);
                return 1;
        }
    }

    switch (argc - optind) {
    case 3: attrname = argv[optind+2];
    case 2: structname = argv[optind+1];
    case 1: corpname = argv[optind];
        break;
    default:
        usage(argv);
        return 1;
    }

    try {
        log() << "opening corpus " << corpname << endl;
        Corpus *corp = new Corpus(corpname);
        std::vector<string> attrnames;

        SubCorpus *subcorp = 0;
        if (not subc.empty()) {
            log() << "opening subcorpus " << subc << endl;
            subcorp = new SubCorpus(corp, subc, false);
        }

        if (base.empty()) {
            if (not subc.empty()) base = subc.substr(0, subc.size() - 5) + ".";
            else base = corp->get_conf("PATH");
        }
        log() << "output prefix is " << base << endl;

        if (!structname.empty() && !attrname.empty()) {
            attrnames.push_back(attrname);
            write_tokenfile(corp, subcorp, structname, attrnames, base);
        } else if (!structname.empty() && attrname.empty()) {
            stringstream ss (corp->get_conf(string(structname) + ".ATTRLIST"));
            while (getline(ss, attrname, ','))
                attrnames.push_back(attrname);
            write_tokenfile(corp, subcorp, structname, attrnames, base);
        } else {
            stringstream sss (corp->get_conf("STRUCTLIST"));
            while (getline(sss, structname, ',')) {
                stringstream ss (corp->get_conf(string(structname) + ".ATTRLIST"));
                attrnames.clear();
                while (getline(ss, attrname, ','))
                    attrnames.push_back(attrname);
                write_tokenfile(corp, subcorp, structname, attrnames, base);
            }
        }

        if (subcorp) delete subcorp;
        delete corp;
    } catch (exception &e) {
        log() << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

