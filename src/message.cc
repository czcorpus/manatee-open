// Copyright (c) 2001-2022  Pavel Rychly

#include <cstring>
#include <cstdio>
#include <cstdarg>
#include "message.hh"

message::message ()
    :outf (stderr), progname (strdup ("")), msg_num (0)
{
}

message::message (const char *prog, const char *id, FILE *out)
    : progname (NULL)
{
    setup (prog, id, out);
}

message::~message ()
{
    delete progname;
}

void message::setup(const char *prog, const char *id, FILE *out)
{
    char s[1024];
    const char *p;
    p = strrchr (prog, '/');
    if (p == NULL)
	p = prog;
    else
	p++;
    strncpy (s, p, sizeof (s) - 1);
    if (id && *id) {
	strncat (s, " ", sizeof (s) - 1 - strlen(s));
	strncat (s, id, sizeof (s) - 1 - strlen(s));
    }
    if (progname)
	delete progname;
    progname = strdup (s);
    outf = out;
    msg_num = 0;
}


void message::operator() (const char *msg ...)
{
    fprintf (outf, "%s[%i]: ", progname, ++msg_num);
    va_list va;
    va_start(va, msg);
    vfprintf (outf, msg, va);
    va_end(va);
    fputc ('\n', outf);
}
