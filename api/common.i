/* -*- C++ -*- */
//  Copyright (c) 1999-2020  Pavel Rychly, Milos Jakubicek

%{
//  Copyright (c) 1999-2020  Pavel Rychly, Milos Jakubicek
#include "concord/concord.hh"
#include "finlib/log.hh"
#include "query/cqpeval.hh"
#include "wordlist.hh"
%}

typedef long long int Position;
typedef long long int NumOfPos;
typedef int ConcIndex;

%include exception.i 
%include std_string.i
%include std_vector.i

%feature("exceptionclass") EvalQueryException;
/** Exception thrown for CQL parsing errors */
class EvalQueryException {
public:
    EvalQueryException (const std::string &name);
    %extend {
        const char* __str__() {
            return self->what();
        }
    }
};

%feature("exceptionclass") IndexError;
/** Exception thrown for index out of range */
%{
class IndexError: public std::exception {
    const std::string _what;
public:
    IndexError (const std::string &where)
        :_what (where + " index out of range") {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~IndexError() throw () {};
};
%}

%feature("exceptionclass") FileAccessError;
/** Exception thrown for failure of a file operation */
class FileAccessError {
public:
    FileAccessError(const char *filename, const char *where);
    %extend {
        const char* __str__() {
            return self->what();
        }
    }
};

%feature("exceptionclass") AttrNotFound;
/** Exception thrown if attribute was not found */
class AttrNotFound {
public:
    AttrNotFound(const std::string &name);
    %extend {
        const char* __str__() {
            return self->what();
        }
    }
};

%feature("exceptionclass") ConcNotFound;
/** Exception thrown if concordance was not found */
class ConcNotFound {
public:
    ConcNotFound(const std::string &name);
    %extend {
        const char* __str__() {
            return self->what();
        }
    }
};

%feature("exceptionclass") CorpInfoNotFound;
/** Exception thrown if corpus configuration information was not found */
class CorpInfoNotFound {
public:
    CorpInfoNotFound(const std::string &name);
    %extend {
        const char* __str__() {
            return self->what();
        }
    }
};

/** Stream of positions */
class FastStream {
public:
    /** Get current position and move to the next one */
    virtual Position next()=0;
    /** Get current position */
    virtual Position peek()=0;
    /** Move to the first position after the indicated one */
    virtual Position find (Position pos) = 0;
    %extend {
        /** Tests end of stream */
        bool end() {
            return self->peek() >= self->final();
        }
        /** Fills in a list of collocations, odd positions are label numbers,
         * even positions are collocation positions relative to KWIC beg */
        void collocs (std::vector<int> &colls) {
            Labels lab;
            self->add_labels (lab);
            for (Labels::iterator it = lab.begin(); it != lab.end(); it++) {
                colls.push_back ((*it).first);
                colls.push_back ((*it).second - self->peek());
            }
        }
    }
};

%exception {
  try {
    $function
  }
  catch (IndexError &e) {
      SWIG_exception(SWIG_IndexError, (char *)e.what());
  }
  catch (std::exception &e) {
      SWIG_exception(SWIG_RuntimeError, (char *)e.what());
  }
}

#ifdef SWIGPYTHON
/* must be extended before %template(StrVector/IntVector/NumVector) */
%extend std::vector<std::string> {
    %pythoncode {
    def __str__ (self):
        if getEncoding():
            s = u"[" + u", ".join(self) + u"]"
            return s.encode(getEncoding())
        else:
            return "[" + ", ".join(self) + "]"
    }
}
%extend std::vector<int> {
    %pythoncode {
    def __str__ (self):
        return "[" + ", ".join(map(str,self)) + "]"
    }
}
%extend std::vector<NumOfPos> {
    %pythoncode {
    def __str__ (self):
        return "[" + ", ".join(map(str,self)) + "]"
    }
}

#endif

%include std_string.i
%include std_vector.i
%include std_pair.i
%template(IntVector)    std::vector<int>;
%template(NumVector)    std::vector<NumOfPos>;
%template(StrVector)    std::vector<std::string>;
%template()    std::vector<Frequency*>;

#ifdef SWIGPYTHON
/* both str and unicode are considered to be string/char* */
%typecheck(SWIG_TYPECHECK_UNISTRING) std::string, char *{
    $1 = (PyString_Check($input) || PyUnicode_Check($input) || $input == Py_None) ? 1 : 0;
}
/* possible input conversion from Unicode */
%fragment("UniToStr", "header") {
    char * UniToStr (PyObject *obj, PyObject **encString, const char *enc) {
        if (!obj) {
            PyErr_SetString(PyExc_ValueError, "UniToStr: NULL pointer passed instead of str, unicode or None object");
            return NULL;
        }
        if (obj == Py_None)
            return NULL;
        if (PyUnicode_Check(obj)) {
            if (enc[0])
                *encString = PyUnicode_AsEncodedString(obj, enc, "replace");
            else
                *encString = PyUnicode_AsEncodedString(obj, "UTF-8", "replace");
            if (!encString)
                PyErr_SetString(PyExc_ValueError, "UniToStr: Failed to decode Unicode string");
            return PyString_AsString(*encString);
        } else {
            return PyString_AsString(obj);
        }
    }
}
%typemap(in, fragment = "UniToStr") char * (PyObject* encString = NULL) {
    $1 = UniToStr($input, &encString, encoding);
}
%typemap(in, fragment = "UniToStr") char (PyObject* encString = NULL) {
    $1 = UniToStr($input, &encString, encoding)[0];
}
%typemap(freearg) char * {
    Py_XDECREF(encString$argnum);
}
%typemap(in) std::string = char*;
%typemap(freearg) std::string = char*;

/* output conversion into Unicode*/
%typemap(out) char* {
if (encoding[0])
    $result = PyUnicode_Decode($1, strlen($1), encoding, "replace");
else
    $result = PyString_FromString($1);
}
%typemap(out) std::string
{
if (encoding[0])
    $result = PyUnicode_Decode($1.c_str(), strlen($1.c_str()), encoding, "replace");
else
    $result = PyString_FromString($1.c_str());
}
%typemap(out) std::vector<std::string> {
    $result = PyTuple_New($1.size());
    for (size_t i = 0; i < $1.size(); i++) {
        PyObject *o = NULL;
        if (encoding[0])
            o = PyUnicode_Decode($1[i].c_str(), strlen($1[i].c_str()), encoding, "replace");
        else
            o = PyString_FromString($1[i].c_str());
        if (!o) {
            $result = NULL;
            break;
        }
        PyTuple_SET_ITEM($result, i, o);
    }
}

/* apply conversions to const qualified variants as well */
%apply char * { const char * };
%apply std::string { const std::string };

/** Sets encoding, if set, Unicode objects are retrieved */
void setEncoding(const char* enc);
const char * getEncoding();
%{
static char encoding[32] = "";
void setEncoding(const char * enc) {
    if (!enc)
        encoding[0] = '\0';
    else {
        strncpy(encoding, enc, 32);
        encoding[31]= '\0';
    }
}
/** Get current encoding */
const char * getEncoding() {return encoding;}
%}
#endif

/* all DisownRS/FS arguments will be automatically disowned */
%inline %{
typedef FastStream DisownFastStream;
typedef RangeStream DisownRangeStream;
%}
%apply SWIGTYPE *DISOWN { DisownFastStream *, DisownRangeStream * };

const std::string version();
%{
const std::string version() {
    std::stringstream version;
    version << finlib_version() << "-" << manatee_version();
    return version.str();
}
%}

%nodefaultctor Frequency;
class Frequency
{
public:
    virtual double freq (int id);
};

/** WordList - see PosAttr for method description */
%nodefaultctor WordList;
class WordList
{
public:
    int id_range ();
    const char* id2str (int id);
    int str2id(const char *str);
    FastStream *id2poss (int id);
    IDPosIterator *idposat (Position pos);
    Generator<int> *regexp2ids (const char *pat, bool ignorecase, const char *filter_pat = NULL);
    IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat = NULL);
    Generator<std::string> *dump_str();
    const std::string attr_path;
    Frequency *get_stat (const char *frqtype) throw (FileAccessError);
};

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
