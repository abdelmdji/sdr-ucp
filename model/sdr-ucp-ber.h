#ifndef SDR_UCP_BER_H
#define SDR_UCP_BER_H

// Pure-math constellation BER model. Deliberately free of any ns-3 dependency
// so it can be unit-tested standalone against textbook reference points.
//
// CONVENTION (this is the part that must not drift):
//   snrLinear is RECEIVED SNR IN THE OCCUPIED BAND, i.e. Es/N0.
//   This is what a link budget Pr / (N0 * B) actually gives you.
//   Symbol rate is held constant across constellations, so Es is constant
//   and the bit rate scales with log2(M). That is the correct model for an
//   acoustic modem with a fixed transducer bandwidth.
//
// Under the Es/N0 convention the Gray-coded AWGN approximations are:
//   BPSK  : Pb = 0.5    * erfc( sqrt( 1.0 * Es/N0 ) )
//   QPSK  : Pb = 0.5    * erfc( sqrt( 0.5 * Es/N0 ) )
//   16-QAM: Pb = 0.375  * erfc( sqrt( 0.1 * Es/N0 ) )
//
// The three leading coefficients and the three scale factors must come from
// the SAME convention. Mixing Eb/N0 and Es/N0 across constellations silently
// grants or removes ~6 dB and is the single easiest way to fake a result.

#include <cmath>
#include <string>

namespace sdrucp {

enum Constellation { BPSK = 0, QPSK = 1, QAM16 = 2 };

inline int BitsPerSymbol (Constellation c)
{
  switch (c) { case BPSK: return 1; case QPSK: return 2; case QAM16: return 4; }
  return 0;
}

inline const char * Name (Constellation c)
{
  switch (c) { case BPSK: return "BPSK"; case QPSK: return "QPSK"; case QAM16: return "16QAM"; }
  return "UNKNOWN";
}

// Bit error probability. snrLinear is Es/N0 (linear, not dB).
inline double Ber (Constellation c, double snrLinear)
{
  if (snrLinear <= 0.0) return 0.5;
  switch (c)
    {
    case BPSK:  return 0.5   * std::erfc (std::sqrt (1.0 * snrLinear));
    case QPSK:  return 0.5   * std::erfc (std::sqrt (0.5 * snrLinear));
    case QAM16: return 0.375 * std::erfc (std::sqrt (0.1 * snrLinear));
    }
  return 0.5;
}

// Packet error probability for an uncoded packet of nBits bits.
// Computed in log space: 1-(1-p)^n loses all precision for small p and large n.
inline double Per (Constellation c, double snrLinear, unsigned long nBits)
{
  const double ber = Ber (c, snrLinear);
  if (ber <= 0.0)  return 0.0;
  if (ber >= 1.0)  return 1.0;
  return -std::expm1 (static_cast<double> (nBits) * std::log1p (-ber));
}

inline double DbToLinear (double db) { return std::pow (10.0, db / 10.0); }
inline double LinearToDb (double lin) { return 10.0 * std::log10 (lin); }

} // namespace sdrucp
#endif
