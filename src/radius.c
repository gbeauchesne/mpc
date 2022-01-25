/* radius -- Functions for radii of complex balls.

Copyright (C) 2022 INRIA

This file is part of GNU MPC.

GNU MPC is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see http://www.gnu.org/licenses/ .
*/

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "mpc-impl.h"

#define MPCR_MANT(r) ((r)->mant)
#define MPCR_EXP(r) ((r)->exp)
#define MPCR_MANT_MIN (((int64_t) 1) << 30)
#define MPCR_MANT_MAX (MPCR_MANT_MIN << 1)

/* The radius can take three types of values, represented as follows:
   infinite: the mantissa is -1 and the exponent is undefined;
   0: the mantissa and the exponent are 0;
   positive: the mantissa is a positive integer, and the radius is
   mantissa*2^exponent. A normalised positive radius "has 31 bits",
   in the sense that the bits 0 to 29 are arbitrary, bit 30 is 1,
   and bits 31 to 63 are 0; otherwise said, the mantissa lies between
   2^30 and 2^31-1.
   Unless stated otherwise, all functions take normalised inputs and
   produce normalised output; they compute only upper bounds on the radii,
   without guaranteeing that these are tight. */


static void mpcr_add_one_ulp (mpcr_ptr r)
   /* Add 1 to the mantissa of the normalised r and renormalise. */
{
   MPCR_MANT (r)++;
   if (MPCR_MANT (r) == MPCR_MANT_MAX) {
      MPCR_MANT (r) >>= 1;
      MPCR_EXP (r)++;
   }
}


static unsigned int leading_bit (int64_t n)
   /* Assuming that n is a positive integer, return the position
      (from 0 to 62) of the leading bit, that is, the k such that
      n >= 2^k, but n < 2^(k+1). */
{
   unsigned int k;

   for (k = 0; n >= 2; n /= 2, k++);

   return k;
}


static void mpcr_normalise_rnd (mpcr_ptr r, mpfr_rnd_t rnd)
   /* The function computes a normalised value for the potentially
      unnormalised r; depending on whether rnd is MPFR_RNDU or MPFR_RNDD,
      the result is rounded up or down. */
{
   unsigned int k;

   if (mpcr_zero_p (r))
      MPCR_EXP (r) = 0;
   else if (!mpcr_inf_p (r)) {
      k = leading_bit (MPCR_MANT (r));
      if (k <= 30) {
         MPCR_MANT (r) <<= 30 - k;
         MPCR_EXP (r) -= 30 - k;
      }
      else {
         MPCR_MANT (r) >>= k - 30;
         MPCR_EXP (r) += k - 30;
         if (rnd == MPFR_RNDU)
            mpcr_add_one_ulp (r);
      }
   }
}


static void mpcr_normalise (mpcr_ptr r)
   /* The potentially unnormalised r is normalised with rounding up. */
{
   mpcr_normalise_rnd (r, MPFR_RNDU);
}


int mpcr_inf_p (mpcr_srcptr r)
{
   return MPCR_MANT (r) == -1;
}


int mpcr_zero_p (mpcr_srcptr r)
{
   return MPCR_MANT (r) == 0;
}


int mpcr_lt_half_p (mpcr_srcptr r)
   /* Return true if r < 1/2, false otherwise. */
{
   return MPCR_EXP (r) < -31;
}


void mpcr_set_inf (mpcr_ptr r)
{
   MPCR_MANT (r) = -1;
}


void mpcr_set_zero (mpcr_ptr r)
{
   MPCR_MANT (r) = 0;
   MPCR_EXP (r) = 0;
}


void mpcr_set_one (mpcr_ptr r)
{
   MPCR_MANT (r) = ((int64_t) 1) << 30;
   MPCR_EXP (r) = -30;
}


void mpcr_set (mpcr_ptr r, mpcr_srcptr s)
{
   r [0] = s [0];
}


int64_t mpcr_get_exp (mpcr_srcptr r)
   /* Return the exponent e such that r = m * 2^e with m such that
      0.5 <= m < 1. */
{
   return MPCR_EXP (r) + 31;
}

void mpcr_out (mpcr_srcptr r)
{
   if (mpcr_inf_p (r))
      printf ("∞");
   else if (mpcr_zero_p (r))
      printf ("Z");
   else {
      printf ("%.20g ", ldexp ((double) MPCR_MANT (r), MPCR_EXP (r)));
      printf ("%li %li", MPCR_MANT (r), MPCR_EXP (r));
   }
}


static void mpcr_mul_rnd (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t,
    mpfr_rnd_t rnd)
    /* Set r to the product of s and t, rounded according to whether rnd
       is MPFR_RNDU or MPFR_RNDD. */
{
   if (mpcr_inf_p (s) || mpcr_inf_p (t))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s) || mpcr_zero_p (t))
      mpcr_set_zero (r);
   else {
      MPCR_MANT (r) = MPCR_MANT (s) * MPCR_MANT (t);
      MPCR_EXP (r) = MPCR_EXP (s) + MPCR_EXP (t);
      mpcr_normalise_rnd (r, rnd);
   }
}


void mpcr_mul (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t)
{
   mpcr_mul_rnd (r, s, t, MPFR_RNDU);
}


static void mpcr_sqr_rnd (mpcr_ptr r, mpcr_srcptr s, mpfr_rnd_t rnd)
    /* Set r to the square of s, rounded according to whether rnd is
       MPFR_RNDU or MPFR_RNDD. */
{
   mpcr_mul_rnd (r, s, s, rnd);
}


void mpcr_sqr (mpcr_ptr r, mpcr_srcptr s)
{
   mpcr_mul_rnd (r, s, s, MPFR_RNDU);
}


static void mpcr_add_rnd (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t,
   mpfr_rnd_t rnd)
    /* Set r to the sum of s and t, rounded according to whether rnd
       is MPFR_RNDU or MPFR_RNDD.
       s and t need not be normalised, but the addition must fit without
       causing an overflow into the sign bit of the mantissae; this is
       in particular the case when the mantissae of s and t start with
       the bits 00, that is, are less than 2^62, for instance because
       they are the results of multiplying two normalised mantissae
       together. */
{
   int64_t d;

   if (mpcr_inf_p (s) || mpcr_inf_p (t))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set (r, t);
   else if (mpcr_zero_p (t))
      mpcr_set (r, s);
   else {
      /* Now all numbers are positive and normalised. */
      d = MPCR_EXP (s) - MPCR_EXP (t);
      if (d >= 0) {
         if (d >= 64)
            /* Shifting by more than the bitlength of the type may cause
               compiler warnings and run time errors. */
            MPCR_MANT (r) = MPCR_MANT (s);
         else
            MPCR_MANT (r) = MPCR_MANT (s) + (MPCR_MANT (t) >> d);
         MPCR_EXP (r) = MPCR_EXP (s);
      }
      else {
         if (d <= -64)
            MPCR_MANT (r) = MPCR_MANT (t);
         else
            MPCR_MANT (r) = MPCR_MANT (t) + (MPCR_MANT (s) >> (-d));
         MPCR_EXP (r) = MPCR_EXP (t);
      }
      if (rnd == MPFR_RNDU)
         MPCR_MANT (r)++;
      mpcr_normalise_rnd (r, rnd);
   }
}


void mpcr_add (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t)
{
   mpcr_add_rnd (r, s, t, MPFR_RNDU);
}


void mpcr_div (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t)
{
   if (mpcr_inf_p (s) || mpcr_inf_p (t))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      if (mpcr_zero_p (t))
         mpcr_set_inf (r);
      else
         mpcr_set_zero (r);
   else {
      MPCR_MANT (r) = (MPCR_MANT (s) << 32) / MPCR_MANT (t) + 1;
      MPCR_EXP (r) = MPCR_EXP (s) - 32 - MPCR_EXP (t);
      mpcr_normalise (r);
   }
}


void mpcr_div_2ui (mpcr_ptr r, mpcr_srcptr s, const unsigned long int e)
{
   if (mpcr_inf_p (s))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set_zero (r);
   else {
      MPCR_MANT (r) = MPCR_MANT (s);
      MPCR_EXP (r) = MPCR_EXP (s) - (int64_t) e;
   }
}


int64_t sqrt_int64 (int64_t n)
   /* Assuming that 2^30 <= n < 2^32, return ceil (sqrt (n*2^30)). */
{
   uint64_t N, s, t;
   int i;

   /* We use the "Babylonian" method to compute an integer square root of N;
      replacing the geometric mean sqrt (N) = sqrt (s * N/s) by the
      arithmetic mean (s + N/s) / 2, rounded up, we obtain an upper bound
      on the square root. */
   N = ((uint64_t) n) << 30;
   s = ((uint64_t) 1) << 31;
   for (i = 0; i < 5; i++) {
      t = s << 1;
      s = (s * s + N + t - 1) / t;
   }

   /* Exhaustive search over all possible values of n shows that with
      6 or more iterations, the method computes ceil (sqrt (N)) except
      for squares N, where it stabilises on sqrt (N) + 1.
      So we need to add a check for s-1; it turns out that then
      5 iterations are enough. */
   if ((s - 1) * (s - 1) >= N)
      return s - 1;
   else
      return s;
}


static void mpcr_sqrt_rnd (mpcr_ptr r, mpcr_srcptr s, mpfr_rnd_t rnd)
    /* Set r to the square root of s, rounded according to whether rnd is
       MPFR_RNDU or MPFR_RNDD. */
{
   if (mpcr_inf_p (s))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set_zero (r);
   else {
      if (MPCR_EXP (s) % 2 == 0) {
         MPCR_MANT (r) = sqrt_int64 (MPCR_MANT (s));
         MPCR_EXP (r) = MPCR_EXP (s) / 2 - 15;
      }
      else {
         MPCR_MANT (r) = sqrt_int64 (2 * MPCR_MANT (s));
         MPCR_EXP (r) = (MPCR_EXP (s) - 1) / 2 - 15;
      }
      /* Now we have 2^30 <= r < 2^31, so r is normalised;
         if r == 2^30, then furthermore the square root was exact,
         so we do not need to subtract 1 ulp when rounding down and
         preserve normalisation. */
      if (rnd == MPFR_RNDD && MPCR_MANT (r) != ((int64_t) 1) << 30)
         MPCR_MANT (r)--;
   }
}


void mpcr_sqrt (mpcr_ptr r, mpcr_srcptr s)
{
   mpcr_sqrt_rnd (r, s, MPFR_RNDU);
}


static void mpcr_set_d_rnd (mpcr_ptr r, double d, mpfr_rnd_t rnd)
   /* Assuming that d is a positive double, set r to d rounded according
      to rnd, which can be one of MPFR_RNDU or MPFR_RNDD. */
{
   double frac;
   int e;

   frac = frexp (d, &e);
   MPCR_MANT (r) = (int64_t) (frac * (((int64_t) 1) << 53));
   MPCR_EXP (r) = e - 53;
   mpcr_normalise_rnd (r, rnd);
}


static void mpcr_mpfr_abs_rnd (mpcr_ptr r, mpfr_srcptr z, mpfr_rnd_t rnd)
   /* Set r to the absolute value of z, rounded according to rnd, which
      can be one of MPFR_RNDU or MPFR_RNDD. */
{
   double d;
   int neg, e;

   neg = mpfr_cmp_ui (z, 0);
   if (neg == 0)
      mpcr_set_zero (r);
   else {
      if (rnd == MPFR_RNDU)
         d = mpfr_get_d (z, MPFR_RNDA);
      else
         d = mpfr_get_d (z, MPFR_RNDZ);
      if (d < 0)
         d = -d;
      mpcr_set_d_rnd (r, d, rnd);
   }
}


void mpcr_mpc_abs (mpcr_ptr r, mpc_srcptr z, mpfr_rnd_t rnd)
    /* Compute a bound on mpc_abs (z) in r.
       rnd can take either of the values MPFR_RNDU and MPFR_RNDD, and
       the function computes an upper or a lower bound, respectively. */
{
   double x, y;
   mpcr_t re, im, u;

   mpcr_mpfr_abs_rnd (re, mpc_realref (z), rnd);
   mpcr_mpfr_abs_rnd (im, mpc_imagref (z), rnd);

   if (mpcr_zero_p (re))
      mpcr_set (r, im);
   else if (mpcr_zero_p (im))
      mpcr_set (r, re);
   else {
      /* Squarings can be done exactly. */
      MPCR_MANT (u) = MPCR_MANT (re) * MPCR_MANT (re);
      MPCR_EXP (u) = 2 * MPCR_EXP (re);
      MPCR_MANT (r) = MPCR_MANT (im) * MPCR_MANT (im);
      MPCR_EXP (r) = 2 * MPCR_EXP (im);
      /* Additions still fit. */
      mpcr_add_rnd (r, r, u, rnd);
      mpcr_sqrt_rnd (r, r, rnd);
   }
}

