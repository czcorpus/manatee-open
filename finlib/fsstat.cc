//  Copyright (c) 2002,2003  Pavel Rychly

#include "fstream.hh"
#include <cmath>

// compute Average Reduced Frequency
double compute_ARF (FastStream *s, int freq, Position size)
{
    if (freq <= 0)
	return 0.0;

    double v = double (size) / freq;
    double sum = 0.0;
    Position prev = s->next();
    double first = prev;
    while (--freq) {
	Position curr = s->next();
	double d = curr - prev;
	prev = curr;
	if (d < v)
	    sum += d / v;
	else
	    sum += 1.0;
    }
    // the first/last interval is cyclic one
    first += size - prev;
    if (first < v)
	sum += first / v;
    else
	sum += 1.0;
    return sum;
}



// compute Average Logarithmic Distance corrected frequency 
double compute_fALD (FastStream *s, Position size)
{
    double N = size;
    double sum = 0.0;
    Position finval = s->final();
    Position prev = s->next();
    if (finval == prev)
	return 0.0;
    double first = prev;
    Position curr;
    while ((curr = s->next()) != finval) {
	double dn = (curr - prev) / N;
	prev = curr;
	sum += dn * log (dn);
    }
    // the first/last interval is cyclic one
    first = (first + N - prev) / N;
    sum += first * log (first);
    return exp (-sum);
}
