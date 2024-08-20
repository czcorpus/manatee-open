//  Copyright (c) 2002,2003  Pavel Rychly

#ifndef FSSTAT_HH
#define FSSTAT_HH

#include "fstream.hh"

// compute Average Reduced Frequency
double compute_ARF (FastStream *s, int freq, Position size);

// compute Average Logarithmic Distance corrected frequency 
double compute_fALD (FastStream *s, Position size);


#endif
