// Sanity check 1: does the BER model reproduce textbook AWGN reference points?
// Run this BEFORE building anything else. If it fails, nothing downstream is trustworthy.
#include "sdr-ucp-ber.h"
#include <cstdio>
#include <cmath>
using namespace sdrucp;

static int failures = 0;

// Invert Ber() to find the Es/N0 (dB) at which a target BER is reached.
static double SnrDbForBer (Constellation c, double targetBer)
{
  double lo = -20.0, hi = 60.0;
  for (int i = 0; i < 200; ++i)
    {
      double mid = 0.5 * (lo + hi);
      if (Ber (c, DbToLinear (mid)) > targetBer) lo = mid; else hi = mid;
    }
  return 0.5 * (lo + hi);
}

static void Check (const char *what, double got, double want, double tolDb)
{
  bool ok = std::fabs (got - want) <= tolDb;
  std::printf ("  %-58s got %7.3f  want %7.3f  %s\n", what, got, want, ok ? "OK" : "*** FAIL ***");
  if (!ok) ++failures;
}

int main ()
{
  std::printf ("\n[1] Es/N0 required for BER = 1e-5 (Gray-coded AWGN)\n");
  // Reference: BPSK needs Eb/N0 = 9.59 dB. Es = Eb, so Es/N0 = 9.59 dB.
  Check ("BPSK  Es/N0 @ BER=1e-5 (dB)", SnrDbForBer (BPSK, 1e-5), 9.59, 0.05);
  // QPSK needs the same Eb/N0 = 9.59 dB, but carries 2 bits/symbol,
  // so Es/N0 = Eb/N0 + 10*log10(2) = 12.60 dB.
  Check ("QPSK  Es/N0 @ BER=1e-5 (dB)", SnrDbForBer (QPSK, 1e-5), 12.60, 0.05);
  // 16-QAM needs Eb/N0 = 13.4 dB, 4 bits/symbol,
  // so Es/N0 = 13.4 + 10*log10(4) = 19.42 dB.
  Check ("16QAM Es/N0 @ BER=1e-5 (dB)", SnrDbForBer (QAM16, 1e-5), 19.42, 0.10);

  std::printf ("\n[2] Equivalent Eb/N0 (should collapse to the classic 9.59 / 9.59 / 13.4)\n");
  for (int k = 0; k < 3; ++k)
    {
      Constellation c = static_cast<Constellation> (k);
      double eb = SnrDbForBer (c, 1e-5) - 10.0 * std::log10 (BitsPerSymbol (c));
      char buf[128]; std::snprintf (buf, sizeof buf, "%-5s Eb/N0 @ BER=1e-5 (dB)", Name (c));
      Check (buf, eb, (k == 2 ? 13.42 : 9.59), 0.10);
    }

  std::printf ("\n[3] Ordering must be strict at every SNR: BPSK <= QPSK <= 16QAM in BER\n");
  bool ordered = true;
  for (double db = -5.0; db <= 40.0; db += 0.25)
    {
      double s = DbToLinear (db);
      if (!(Ber (BPSK, s) <= Ber (QPSK, s) + 1e-18 && Ber (QPSK, s) <= Ber (QAM16, s) + 1e-18))
        { ordered = false; std::printf ("  violated at %.2f dB\n", db); break; }
    }
  std::printf ("  monotone robustness ordering over -5..40 dB           %s\n", ordered ? "OK" : "*** FAIL ***");
  if (!ordered) ++failures;

  std::printf ("\n[4] PER must not saturate to 0 or 1 through the useful region\n");
  const unsigned long nbits = 128UL * 8UL;
  int transition = 0;
  for (double db = 0.0; db <= 35.0; db += 0.25)
    { double p = Per (QPSK, DbToLinear (db), nbits); if (p > 1e-3 && p < 1.0 - 1e-3) ++transition; }
  std::printf ("  QPSK 128B PER has %d intermediate points in 0..35 dB   %s\n",
               transition, transition >= 8 ? "OK" : "*** FAIL ***");
  if (transition < 8) ++failures;

  std::printf ("\n[5] log-space PER must not lose precision at tiny BER\n");
  double p = Per (BPSK, DbToLinear (25.0), nbits);
  std::printf ("  BPSK 128B PER @ 25 dB = %.6e                          %s\n",
               p, (p > 0.0 && p < 1e-9) ? "OK" : "*** FAIL ***");
  if (!(p > 0.0 && p < 1e-9)) ++failures;

  std::printf ("\n[6] What the thesis code used, evaluated in this convention\n");
  std::printf ("  distance-figure set  BPSK 1.0 / QPSK 0.5 / 16QAM 0.4\n");
  std::printf ("     -> 16QAM scale 0.4 vs correct 0.1  = %+.2f dB gifted to 16QAM\n",
               10.0 * std::log10 (0.4 / 0.1));
  std::printf ("  mobility/density set BPSK 1.0 / QPSK 0.12 / 16QAM 0.105\n");
  std::printf ("     -> QPSK scale 0.12 vs correct 0.5  = %+.2f dB taken from QPSK\n",
               10.0 * std::log10 (0.12 / 0.5));

  std::printf ("\n%s (%d failure%s)\n\n", failures ? "SELFTEST FAILED" : "SELFTEST PASSED",
               failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
