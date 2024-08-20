//  Copyright (c) 1999-2013  Pavel Rychly, Milos Jakubicek


#include "conccrit.hh"
#include "utf8.hh"
#include <sstream>
#include <cstring>
#include <cstdlib>

#ifdef USE_ICU
#include <unicode/ucnv.h>
#include <unicode/unistr.h>
#include <unicode/locid.h>
#include <unicode/coll.h>
#include <unicode/sortkey.h>
using namespace icu;
#else
#include <clocale>
#endif


using namespace std;

static const char *utf82lower (const unsigned char *str, 
                               const char *, const char *)
{
    return (const char *) utf8_tolower (str);
}

#ifdef USE_ICU

static const char *str2lower (const char *str, const char *locale,
                        const char *encoding)
{
    // viz ~/inst/icu.old/data/convrtrs.txt
    UErrorCode err = U_ZERO_ERROR;
    UConverter *conv = ucnv_open (encoding, &err);
    const Locale loc(locale);
    UnicodeString us (str, -1, conv, err);
    //XXX if (U_FAILURE (err)) ...
    us.toLower (loc);
    static char *cbuff = NULL;
    static int cbuffsize = 0;

    int len = us.extract (cbuff, cbuffsize, conv, err) +1 ;
    if (cbuffsize < len) {
        cbuffsize = len;
        cbuff = (char *)realloc (cbuff, cbuffsize);
        //XXX test returned value
        us.extract (cbuff, cbuffsize, conv, err);
    }
    
    ucnv_close (conv);
    return cbuff;
}

 // USE_ICU
#elif defined BUILDIN_PCRE_TABLES
#include "regpref.hh"

static const char *str2lower (const char *str, const char *locale, 
                        const char *encoding)
{
    const char *tab = (char*) get_pcre_lower_table (locale);
    if (!tab) 
        return str;

    static char *buff = NULL;
    static unsigned buffsize = 0;
    if (buffsize <= strlen (str)) {
        buffsize = strlen (str) +1;
        buff = (char *)realloc (buff, buffsize);
        //XXX test returned value
    }
    
    char *p = buff;
    for (; *str; str++, p++)
        *p = tab [(unsigned char)*str];
    *p = '\0';
    return buff;
}

#else // BUILDIN_PCRE_TABLES

static const char *str2lower (const char *str, const char *locale, 
                        const char *encoding)
{
    static char *buff = NULL;
    static unsigned buffsize = 0;
    if (buffsize <= strlen (str)) {
        buffsize = strlen (str) +1;
        buff = (char *)realloc (buff, buffsize);
        //XXX test returned value
    }
    
    const char *prev_locale = setlocale (LC_CTYPE, locale);
    char *p = buff;
    for (; *str; str++, p++)
        *p = tolower(*str);
    setlocale (LC_CTYPE, prev_locale);
    *p = '\0';
    return buff;
}
#endif //else BUILDIN_PCRE_TABLES

#ifdef USE_ICU
static char *str2retro (const char *str, const char *encoding)
{
    UErrorCode err = U_ZERO_ERROR;
    UConverter *conv = ucnv_open (encoding, &err);
    UnicodeString us (str, -1, conv, err);
    //XXX if (U_FAILURE (err)) ...
    us.reverse();

    static char *cbuff = NULL;
    static unsigned cbuffsize = 0;
    unsigned len = us.extract (cbuff, cbuffsize, conv, err) +1;
    if (cbuffsize < len) {
        cbuffsize = len;
        cbuff = (char *)realloc (cbuff, cbuffsize);
        //XXX test returned value
        us.extract (cbuff, cbuffsize, conv, err);
    }
    ucnv_close (conv);
    return cbuff;
}

#else

static char *str2retro (const char *str, const char *)
{
    static char *buff = NULL;
    static unsigned buffsize = 0;
    unsigned len = strlen (str);
    if (buffsize <= len) {
        buffsize = len +1;
        buff = (char *)realloc (buff, buffsize);
        //XXX test returned value
    }
    char *p = buff + len;
    *p-- = '\0';
    for (; *str; str++, p--)
        *p = *str;
    return buff;
}
#endif

#ifdef USE_ICU
static const char *str2locale (const char *str, const char *locale,
                         const char *encoding)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = ucnv_open (encoding, &status);
    const Locale loc(locale);
    UnicodeString us (str, -1, conv, status);

    Collator* coll = Collator::createInstance(loc, status);
    if (U_FAILURE(status)) return "xx1";
    coll->setStrength(Collator::PRIMARY);
    CollationKey key;
    coll->getCollationKey (us, key, status);
    if (U_FAILURE(status)) return "xx2";

    int32_t required;
    const uint8_t* out = key.getByteArray (required);
    static char *cbuff = NULL;
    static int cbuffsize = 0;
    if (cbuffsize <= required) {
        cbuffsize = required+1;
        cbuff = (char *)realloc (cbuff, cbuffsize);
        //XXX test returned value
    }
    strncpy (cbuff, (char*)out, required);
    cbuff [required] = 0;
    return cbuff;
}

// ICU_LOCALE
#elif BUILDIN_PCRE_TABLES

static const char *str2locale (const char *str, const char *locale,
                         const char *encoding)
{
    const char *tab = (char*) get_pcre_collate_table (locale);
    if (!tab) 
        return str;

    static char *buff = NULL;
    static unsigned buffsize = 0;
    unsigned len = strlen (str)*16;
    if (buffsize <= len) {
        buffsize = len +1;
        buff = (char *)realloc (buff, buffsize);
        //XXX test returned value
    }

    char *p = buff;
    while (*str) {
        const char *base = tab + (unsigned char)(*str) *16;
        char *q = (char *)memccpy (p, base, 0, 16);
        p = q ? q - 1 : p + 16;
        str++;
    }
    *p = 0;
    return buff;
}

#else //BUILDIN_PCRE_TABLES

static const char *str2locale (const char *str, const char *locale,
                         const char *encoding)
{
    static char *buff = NULL;
    static unsigned buffsize = 0;
    unsigned len;
    const char *prev_locale = setlocale (LC_COLLATE, locale);
    if (buffsize <= (len = strxfrm (buff, str, buffsize))) {
        buffsize = len +1;
        buff = (char *)realloc (buff, buffsize);
        //XXX test returned value
        strxfrm (buff, str, buffsize);
    }
    setlocale (LC_COLLATE, prev_locale);
    return buff;
}

#endif // else BUILDIN_PCRE_TABLES


// ==================================================
// ==================== criteria ====================

Concordance::criteria::~criteria()
{
}

PosAttr *Concordance::criteria::get_attr()
{
    return NULL;
}

typedef const char * tolower_fn_t(const char *, const char *, const char *);

class criteria_base : public Concordance::criteria {
protected:
    bool ignore_case;
    bool retrograde;
    bool empty_endbeg;
    bool output_ids;
    const char *locale;
    const char *encoding;
    PosAttr *pa;
    tolower_fn_t *tolower_fn;

    criteria_base (Corpus *c, RangeStream *r, string attr_opt)
        : ignore_case (false), retrograde (false), empty_endbeg (false),
        output_ids (false), locale (NULL),
        encoding (c->get_conf ("ENCODING").c_str()) {
        strip_options (attr_opt);
        pa = c->get_attr (attr_opt);
        locale = pa->locale; // set default attribute locale
        if (attr_opt.find('.') != string::npos
            && CorpInfo::str2bool(c->get_conf(pa->name + ".MULTIVALUE")))
            multisep = c->get_conf(pa->name + ".MULTISEP").c_str();
    }
    criteria_base (): ignore_case (false), retrograde (false), locale (NULL),
                      encoding (NULL), pa (NULL) {}
public:
    virtual ~criteria_base() {}
    virtual const char *get_str (RangeStream *r) =0;
    virtual void push (RangeStream *r, vector<string> &v) {
        v.push_back (get (r));
    }
    void strip_options (string &attr_opt) {
        int slash = attr_opt.find ('/');
        if (slash >= 0) {
            ignore_case = retrograde = empty_endbeg = output_ids = false;
            for (unsigned i = slash +1; i < attr_opt.length(); i++) {
                switch (attr_opt[i]) {
                case 'e':
                    empty_endbeg = true; break;
                case 'i':
                    ignore_case = true; 
                    if (!strcmp(encoding,"UTF-8"))
                        tolower_fn = (tolower_fn_t*) utf82lower;
                    else
                        tolower_fn = str2lower;
                    break;
                case 'r':
                    retrograde = true; break;
                case 'n':
                    output_ids = true; break;
                case 'L':
                    {
                        string loc (attr_opt, i+1);
                        locale = locale2c_str (loc);
                        i+= loc.length();
                    }
                    break;
                default:
                    cerr << "incorrect criteria option `" << attr_opt[i] 
                         << "'\n";
                }
            }
            attr_opt.replace (slash, attr_opt.length() - slash, "");
        }
    }
    virtual const char *get (RangeStream *r, bool case_retro=false)
    {
        return apply_opts (get_str(r), case_retro);
    }
    const char *apply_opts (const char *str, bool case_retro=false) {
        if (ignore_case)
            str = tolower_fn (str, locale, encoding);
        if (retrograde)
            str = str2retro (str, encoding);
        if (case_retro)
            return str;
        if (locale)
            str = str2locale (str, locale, encoding);
        return str;
    }
    virtual PosAttr *get_attr() {
        return pa;
    }
};

// -------------------- crit_struct_nr -------------------
class crit_struct_nr: public criteria_base {
    Structure *str;
public:
    crit_struct_nr (Corpus *c, RangeStream *r, const string &str) :
        str(c->get_struct (str)) {}
    virtual ~crit_struct_nr() {}
    virtual const char *get_str (RangeStream *r) {
        static string buff;
        Position n = str->rng->num_at_pos (r->peek_beg());
        if (n == -1) {
            buff = "";
            return buff.c_str();
        }
        stringstream ss;
        ss << str->name << '#' << n;
        buff = ss.str();
        return buff.c_str();
    }
};

// -------------------- crit_pos_attr --------------------
class crit_pos_attr : public criteria_base {
    Concordance::context *ctx;
public:
    virtual const char *get_str (RangeStream *r) {
        static char buff[10];
        Position pos = ctx->get (r);
        if (output_ids) {
            snprintf (buff, 10, "%d", pa->pos2id (pos));
            return buff;
        }
        else
            return pa->pos2str (pos);
    }
    crit_pos_attr (Corpus *c, RangeStream *r, const string &attr,
                   const char *ctxstr)
        : criteria_base (c, r, attr),
          ctx (prepare_context (c, ctxstr, true)) {}
    virtual ~crit_pos_attr() {
        delete ctx;
    }
};

// -------------------- crit_range --------------------
class crit_range : public criteria_base {
    Concordance::context *beg;
    Concordance::context *end;
public:
    crit_range (Corpus *c, RangeStream *r, const string &attr,
                const char *ctxbeg, const char *ctxend)
        : criteria_base (c, r, attr),
          beg (prepare_context (c, ctxbeg, true)),
          end (prepare_context (c, ctxend, false))
    {}
    virtual ~crit_range() {
        delete beg;
        delete end;
    }
    virtual const char *get_str (RangeStream *r) {
        static string buff;
        ostringstream oss;
        Position b = beg->get (r);
        Position e = end->get (r);
        TextIterator *ti = NULL;
        IDIterator *ii = NULL;
        if (b <= e) {
            if (output_ids)
                ii = pa->posat (b);
            else
                ti = pa->textat (b);
            for (; b <= e; b++) {
                if (output_ids)
                    oss << ii->next();
                else
                    oss << ti->next();
                oss << separator;
            }
        } else {
            if (empty_endbeg) return "";
            if (output_ids)
                ii = pa->posat (e);
            else
                ti = pa->textat (e);
            for (; e <= b; e++) {
                string tmp = oss.str();
                oss.str("");
                if (output_ids)
                    oss << ii->next();
                else
                    oss << ti->next();
                oss << separator << tmp;
            }
        }
        buff = oss.str();
        if (!buff.empty())
            buff.replace (buff.length() -1, 1, "");
        delete ti;
        delete ii;
        return buff.c_str();
    }
    virtual void push (RangeStream *r, vector<string> &v) {
        Position b = beg->get (r);
        Position e = end->get (r);
        TextIterator *ti = NULL;
        IDIterator *ii = NULL;
        char buff[10];
        if (b <= e) {
            if (output_ids)
                ii = pa->posat (b);
            else
                ti = pa->textat (b);
            for (; b <= e; b++) {
                if (output_ids) {
                    snprintf (buff, 10, "%d", ii->next());
                    v.push_back (buff);
                } else
                    v.push_back (apply_opts (ti->next()));
            }
        } else {
            if (output_ids)
                ii = pa->posat (e);
            else
                ti = pa->textat (e);
            vector<string> tmp;
            for (; e <= b; e++) {
                if (output_ids) {
                    snprintf (buff, 10, "%d", ii->next());
                    tmp.push_back (buff);
                } else
                    tmp.push_back (apply_opts (ti->next()));
            }
            v.insert (v.end(), tmp.rbegin(), tmp.rend());
        }
        delete ti;
        delete ii;
    }
};

// -------------------- crit_linegroup --------------------
class crit_linegroup : public criteria_base {
public:
    virtual const char *get_str (RangeStream *r) {
        static char grp[3] = "  ";
        Labels lab;
        r->add_labels (lab);
        int g = lab[Concordance::lngroup_labidx];
        if (g) {
            grp[0] = g < 10 ? ' ' : '0' + g / 10;
            grp[1] = '0' + g % 10;
        } else {
            grp[0] = '?';
            grp[1] = '\0';
        }
        return grp;
    }
    crit_linegroup (Corpus *c, RangeStream *r)
        : criteria_base (c, r, "-"){}
    virtual ~crit_linegroup() {}
};

// ==================== prepare_criteria ====================
void 
prepare_criteria (Corpus *c, RangeStream *r, const char *critstr,
                  vector<Concordance::criteria*> &vec)
{
    istringstream in (critstr);
    string attrs, ctxs;

    while (in >> attrs >> ctxs) {
        int i;
        if (attrs == "^") {
            vec.push_back (new crit_linegroup (c, r));
        } else if ((i = ctxs.find ('~')) >= 0) {
            string left (ctxs, 0, i);
            string right (ctxs, i+1);
            vec.push_back (new crit_range (c, r, attrs, left.c_str(),
                                          right.c_str()));
        } else {
            try {
                vec.push_back (new crit_pos_attr (c, r, attrs, ctxs.c_str()));
            } catch (AttrNotFound&) {
                vec.push_back (new crit_struct_nr (c, r, attrs));
            }
        }
    }
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

