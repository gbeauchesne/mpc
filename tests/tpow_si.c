/* tpow_si -- test file for mpc_pow_si.

Copyright (C) 2009, 2010, 2011, 2012 INRIA

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

#include <limits.h> /* for CHAR_BIT */
#include "mpc-tests.h"

static void
compare_mpc_pow (mpfr_prec_t pmax, int iter, unsigned long nbits)
   /* copied from tpow_ui.c and replaced unsigned by signed */
{
  mpfr_prec_t p;
  mpc_t x, y, z, t;
  long n;
  int i, inex_pow, inex_pow_si;
  mpc_rnd_t rnd1, rnd2, rnd;

  mpc_init3 (y, sizeof (unsigned long) * CHAR_BIT, MPFR_PREC_MIN);
  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    for (i = 0; i < iter; i++)
      {
        mpc_init2 (x, p);
        mpc_init2 (z, p);
        mpc_init2 (t, p);
        mpc_urandom (x, rands);
        n = (signed long) gmp_urandomb_ui (rands, nbits);
        mpc_set_si (y, n, MPC_RNDNN);
        for (rnd1 = 0; rnd1 < 4; rnd1 ++)
          for (rnd2 = 0; rnd2 < 4; rnd2 ++)
          {
            rnd = MPC_RND (rnd1, rnd2);
            inex_pow = mpc_pow (z, x, y, rnd);
            inex_pow_si = mpc_pow_si (t, x, n, rnd);
            if (mpc_cmp (z, t) != 0)
              {
                printf ("mpc_pow and mpc_pow_si differ for x=");
                mpc_out_str (stdout, 10, 0, x, MPC_RNDNN);
                printf (" n=%li p=%lu rnd=%d\n", n, p, rnd);
                printf ("mpc_pow gives ");
                mpc_out_str (stdout, 10, 0, z, MPC_RNDNN);
                printf ("\nmpc_pow_si gives ");
                mpc_out_str (stdout, 10, 0, t, MPC_RNDNN);
                printf ("\n");
                exit (1);
              }
            if (inex_pow != inex_pow_si)
              {
                printf ("mpc_pow and mpc_pow_si give different flags for x=");
                mpc_out_str (stdout, 16, 0, x, MPC_RNDNN);
                printf (" n=%li p=%lu rnd=%d\n", n, p, rnd);
                printf ("mpc_pow gives %d\n", inex_pow);
                printf ("mpc_pow_si gives %d\n", inex_pow_si);
                exit (1);
              }
          }
        mpc_clear (x);
        mpc_clear (z);
        mpc_clear (t);
      }
  mpc_clear (y);
}

#define MPC_FUNCTION_CALL                                               \
  P[0].mpc_inex = mpc_pow_si (P[1].mpc, P[2].mpc, P[3].si, P[4].mpc_rnd)
#define MPC_FUNCTION_CALL_REUSE_OP1                                     \
  P[0].mpc_inex = mpc_pow_si (P[1].mpc, P[1].mpc, P[3].si, P[4].mpc_rnd)

#include "data_check.tpl"

int
main (void)
{
  test_start ();

  data_check_template ("pow_si.dsc", "pow_si.dat");

  compare_mpc_pow (100, 5, 19);

  test_end ();

  return 0;
}
