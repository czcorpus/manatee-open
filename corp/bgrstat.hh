// Copyright (c) 2000-2008  Pavel Rychly

#ifndef BGRSTAT_HH
#define BGRSTAT_HH

typedef double bigram_fun (double, double, double, double);

bigram_fun* code2bigram_fun (char c);

extern const char *bgr_known_fun_codes;

double bgr_t_score (double f_AB, double f_A, double f_B, double N);

double bgr_mi_score (double f_AB, double f_A, double f_B, double N);

double bgr_mi3_score (double f_AB, double f_A, double f_B, double N);

double bgr_log_likelihood (int f_AB, int f_A, int f_B, int N);
double bgr_log_likelihood_bf (double f_AB, double f_A, double f_B, double N);

double bgr_minimum_sensitivity (double f_AB, double f_A, double f_B);
double bgr_minimum_sensitivity_bf (double f_AB, double f_A, double f_B, double);

double bgr_prod_mi_log_freq (double f_AB, double f_A, double f_B, double N);

double bgr_relative_freq (double f_AB, double f_A, double, double);

double bgr_log_dice (double f_AB, double f_A, double f_B, double);

#endif
