//  Copyright (c) 1999-2014  Pavel Rychly

#include "dynfun.hh"
#include "utf8.hh"
#include "excep.hh"
#include <cstdio>
#include <cstdarg>
#include <dlfcn.h>

//-------------------- internal_funs --------------------

struct fun_item {
    const char *name;
    void *fn;
};
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <iconv.h>

#include <string>
#include <unordered_set>
#include <vector>

#include <system_error>
#include <regex>

extern "C" {
#include "stddynfun.c"
}

#define DEFFN(x) {#x, (void *)x}
static fun_item internal_funs [] = {
    DEFFN(getnchar),
    DEFFN(getnextchar),
    DEFFN(getnextchars),
    DEFFN(getfirstn),
    DEFFN(getfirstbysep),
    DEFFN(getnbysep),
    DEFFN(getlastn),
    DEFFN(utf8getlastn),
    DEFFN(striplastn),
    DEFFN(lowercase),
    DEFFN(utf8lowercase),
    DEFFN(utf8uppercase),
    DEFFN(utf8capital),
    DEFFN(url2domain),
    DEFFN(url3domain),
    DEFFN(ascii),
    {NULL, NULL}
};
#undef DEFFN

//-------------------- DynFun_pipe --------------------

class DynFun_pipe: public DynFun {
    std::string cmd;
    char *line = 0;
    size_t linesize = 0;
    std::regex quote_re;
public:
    DynFun_pipe(const char *command) : cmd(command), quote_re("'") {};

    virtual const char *operator() (const char *x) {
#if GCC_VERSION < 40900
        std::string replace = "'\\''", search = "'", escaped = x;
        size_t pos = 0;
        while((pos = escaped.find(search, pos)) != std::string::npos) {
            escaped.replace(pos, search.length(), replace);
            pos += replace.length();
        }
#else
        std::string escaped = std::regex_replace(x, quote_re, "'\\''");
#endif
        std::string command = "echo '" + escaped + "'|" + cmd;
        FILE *fd = popen(command.c_str(), "r");
        if(!fd) throw std::system_error(errno, std::generic_category());
        int ret = getline(&line, &linesize, fd);
        if(ret < 0) {  // no output or I/O error
            pclose(fd);
            if(errno) throw std::system_error(errno, std::generic_category());
            throw std::runtime_error("no output from dynamic attribute pipeline");
        }
        char *eolpos;
        if((eolpos = strchr(line, '\n'))) *eolpos = 0;
        pclose(fd);  // XXX can fail, print warning?
        return line;
    };
    virtual ~DynFun_pipe() {
        free(line);
    };
};

//-------------------- DynFun_base --------------------

class DynFun_base: public DynFun {
protected:
    void *fn;
    void *dllhandle;
public:
    DynFun_base (const char *libpath, const char *funname);
    virtual ~DynFun_base();
    const char * operator () (const char *x) = 0;
};

DynFun_base::DynFun_base (const char *libpath, const char *funname)
{
    if (strcmp (libpath, "internal") == 0) {
        dllhandle = 0;
        for (fun_item *i=internal_funs; i->name != NULL; i++)
            if (strcmp (funname, i->name) == 0) {
                fn = i->fn;
                return;
            }
        throw CorpInfoNotFound(std::string("Cannot find internal function ")
                               + funname);
    } else {
        int errors = 0;
        /* Load the module. */
        if (!errors)
            dllhandle = dlopen (libpath, RTLD_LAZY);
        if (dllhandle) {
            fn = dlsym (dllhandle, funname);
            if (fn == NULL) {
                fprintf(stderr, "Cannot load dynamic function %s: %s\n",
                                funname, dlerror());
                errors = dlclose (dllhandle);
                dllhandle = NULL;
            }
        }
        if (errors) fprintf (stderr, "%s\n", dlerror());
    }
}



DynFun_base::~DynFun_base() 
{
    if (dllhandle)
        dlclose (dllhandle);
}



//-------------------- DynFun_* --------------------

class DynFun_0: public DynFun_base {
public:
    typedef const char * (*type)(const char *);
    DynFun_0 (const char *libpath, const char *funname)
        : DynFun_base(libpath, funname) {}
    virtual const char * operator() (const char *x)
        {return type(fn)(x);}
};

template <class ArgType>
class DynFun_1: public DynFun_base {
public:
    typedef const char * (*type)(const char *, ArgType);
    ArgType op;
    DynFun_1 (const char *libpath, const char *funname, ArgType op)
        : DynFun_base(libpath, funname), op(op) {}
    virtual const char * operator() (const char *x)
        {return type(fn)(x, op);}
};

template <class Arg1Type, class Arg2Type>
class DynFun_2: public DynFun_base {
public:
    typedef const char * (*type)(const char *, Arg1Type, Arg2Type);
    Arg1Type op1;
    Arg2Type op2;
    DynFun_2 (const char *libpath, const char *funname, 
              Arg1Type op1, Arg2Type op2)
        : DynFun_base(libpath, funname), op1(op1), op2(op2) {}
    virtual const char * operator() (const char *x)
        {return type(fn)(x, op1, op2);}
};


typedef DynFun_1<int> DynFun_i;
typedef DynFun_1<char> DynFun_c;
typedef DynFun_1<const char*> DynFun_s;
typedef DynFun_2<int, int> DynFun_ii;
typedef DynFun_2<int, char> DynFun_ic;
typedef DynFun_2<int, const char*> DynFun_is;
typedef DynFun_2<char, int> DynFun_ci;
typedef DynFun_2<char, char> DynFun_cc;
typedef DynFun_2<char, const char*> DynFun_cs;
typedef DynFun_2<const char*, int> DynFun_si;
typedef DynFun_2<const char*, char> DynFun_sc;
typedef DynFun_2<const char*, const char*> DynFun_ss;


//-------------------- createDynFun --------------------

DynFun *createDynFun (const char *type, const char *libpath, 
                      const char *funname, ...)
{
    if (strcmp(libpath, "pipe") == 0)
        return new DynFun_pipe (funname);

    if (type[0] == '\0' || (type[1] == '\0' && type[0] == '0'))
        return new DynFun_0 (libpath, funname);

    const char *arg1, *arg2;
    va_list va;
    va_start(va, funname);
    arg1 = va_arg (va, const char *);
    if (type[1] == '\0') {
        va_end(va);
        switch (type[0]) {
        case 's':
            return new DynFun_s (libpath, funname, strdup (arg1));
        case 'i':
            return new DynFun_i (libpath, funname, atol (arg1));
        case 'c':
            return new DynFun_c (libpath, funname, arg1[0]);
        }
    } else {
        arg2 = va_arg (va, const char *);
        va_end(va);
        switch (type[0]) {
        case 's':
            switch (type[1]) {
            case 's':
                return new DynFun_ss (libpath, funname, 
                                      strdup (arg1), strdup (arg2)); 
            case 'i':
                return new DynFun_si (libpath, funname, 
                                      strdup (arg1), atol (arg2));
            case 'c':
                return new DynFun_sc (libpath, funname, 
                                      strdup (arg1), arg2[0]);
            }
            break;
        case 'i':
            switch (type[1]) {
            case 's':
                return new DynFun_is (libpath, funname, 
                                      atol (arg1), strdup (arg2)); 
            case 'i':
                return new DynFun_ii (libpath, funname, atol (arg1), atol (arg2));
            case 'c':
                return new DynFun_ic (libpath, funname, atol (arg1), arg2[0]);
            }
            break;
        case 'c':
            switch (type[1]) {
            case 's':
                return new DynFun_cs (libpath, funname, arg1[0], strdup (arg2));
            case 'i':
                return new DynFun_ci (libpath, funname, arg1[0], atol (arg2));
            case 'c':
                return new DynFun_cc (libpath, funname, arg1[0], arg2[0]);
            }
            break;
        }
    }
    return NULL;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
