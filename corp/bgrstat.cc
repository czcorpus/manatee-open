// Copyright (c) 2000-2016  Pavel Rychly, Milos Jakubicek

#include "bgrstat.hh"
#include <cmath>


// T-Score

double bgr_t_score (double f_AB, double f_A, double f_B, double N)
{
    return (f_AB - f_A * f_B / N) / sqrt (f_AB);
}


// MI-Score from:
//
// Kenneth Ward Church and Patrick Hanks, Word Association Norms,
// Mutual Information, and Lexicography, in Computational Linguistics,
// 16(1):22-29, 1990

double bgr_mi_score (double f_AB, double f_A, double f_B, double N)
{
    return log (f_AB * N / (f_A * f_B)) / log(2.0);
}

// MI^3-Score from:
//
// XXX realy?
// Michael P. Oakes, Statistics for Corpus Linguistics, 1998

double bgr_mi3_score (double f_AB, double f_A, double f_B, double N)
{
    return log (f_AB * f_AB * f_AB * N / (f_A * f_B)) / log(2.0);
}


// Log Likelihood from:
// 
// Ted Dunning, Accurate Methods for the Statistics of Surprise and
// Coincidence, 1993

static inline double xlx (double x) {
    return x <= 0.0 ? 0.0 : x * log (x);
}


double bgr_log_likelihood (int f_AB, int f_A, int f_B, int N)
{
    return 2 * (xlx(f_AB) + xlx(f_A - f_AB) + xlx(f_B - f_AB) + xlx(N)
		+ xlx (N + f_AB - f_A - f_B)
		- xlx(f_A) - xlx(f_B) - xlx(N - f_A) - xlx(N - f_B));
}

double bgr_log_likelihood_bf (double f_AB, double f_A, double f_B, double N)
{
    return 2 * (xlx(f_AB) + xlx(f_A - f_AB) + xlx(f_B - f_AB) + xlx(N)
		+ xlx (N + f_AB - f_A - f_B)
		- xlx(f_A) - xlx(f_B) - xlx(N - f_A) - xlx(N - f_B));
}


// Minimum Sensitivity from:
//
// Ted Pedersen, Dependent Bigram Identification, in Proc. of the
// Fifteenth National Conference on Artificial Intelligence, 1998

double bgr_minimum_sensitivity (double f_AB, double f_A, double f_B)
{
    double sw1 = f_AB / f_B;
    double sw2 = f_AB / f_A;
    return sw1 <= sw2 ? sw1 : sw2;
}

double bgr_minimum_sensitivity_bf (double f_AB, double f_A, double f_B, double)
{
    return bgr_minimum_sensitivity (f_AB, f_A, f_B);
}

double bgr_dice (double f_AB, double f_A, double f_B, double)
{
    return 100.0 * 2.0 * f_AB / (f_A + f_B);
}

double bgr_log10_dice (double f_AB, double f_A, double f_B, double)
{
    return 5.0 + log(2.0 * f_AB / (f_A + f_B)) / log(10.0);
}

double bgr_log_dice (double f_AB, double f_A, double f_B, double)
{
    return 14.0 + log(2.0 * f_AB / (f_A + f_B)) / log(2.0);
}



// Salience from:
// Adam Kilgariff, David Tugwell, .....

double bgr_prod_mi_log_freq (double f_AB, double f_A, double f_B, double N)
{
    return bgr_mi_score (f_AB, f_A, f_B, N) * log (f_AB +1);
}

// TODO: find appropriate name
double bgr_prod_mi_rel (double f_AB, double f_A, double f_B, double N)
{
    // MI * log (freq per 1M tokens)
    return bgr_mi_score (f_AB, f_A, f_B, N) * (log (1.5 + f_AB * 1000000.0 / N));
}

double bgr_relative_freq (double f_AB, double f_A, double, double)
{
    return (f_AB / f_A) * 100.0;
}

double bgr_abs_freq (double f_AB, double, double, double)
{
    return f_AB;
}

double bgr_abs_freq_coll (double, double f_A, double, double)
{
    return f_A;
}

double bgr_null (double, double, double, double)
{
    return 0.0;
}

//-------------------- code2bigram_fun --------------------

const char *bgr_known_fun_codes = "tm3lsprfCDd1";

bigram_fun* code2bigram_fun (char c)
{
    switch (c) {
    case 't': return &bgr_t_score;
    case 'm': return &bgr_mi_score;
    case '3': return &bgr_mi3_score;
    case 'l': return &bgr_log_likelihood_bf;
    case 's': return &bgr_minimum_sensitivity_bf;
    case 'p': return &bgr_prod_mi_log_freq;
    case 'r': return &bgr_relative_freq;
    case 'f': return &bgr_abs_freq;
    case 'F': return &bgr_abs_freq_coll;
    case 'C': return &bgr_prod_mi_rel;
    case 'D': return &bgr_dice;
    case 'd': return &bgr_log_dice;
    case '1': return &bgr_log10_dice;
    }
    return &bgr_null;
}
