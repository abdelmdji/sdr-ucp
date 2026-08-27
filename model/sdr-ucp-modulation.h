#ifndef SDR_UCP_MODULATION_H
#define SDR_UCP_MODULATION_H
#include "ns3/aqua-sim-modulation.h"
#include "ns3/sdr-ucp-ber.h"
namespace ns3 {
/* One constellation (BPSK / QPSK / 16-QAM) as an AquaSimModulation.
 *
 * Upstream AquaSimModulation has a CONSTANT m_ber attribute and its Per()
 * ignores SNR entirely; the receive path never calls Per() at all, because
 * AquaSimPhyCmn::Decodable() defers to a hard SINR threshold. So out of the
 * box, changing modulation changes only TxTime, never whether a packet
 * survives. This class adds the SNR-dependent part; SdrUcpPhy calls it.
 *
 * SPS (symbols/second) is held CONSTANT across the three constellations, so
 * the bit rate scales with log2(M) and the symbol energy does not. That is the
 * fixed-bandwidth acoustic modem case and it is what makes Es/N0 the right
 * SNR convention (see sdr-ucp-ber.h).
 */
class SdrUcpModulation : public AquaSimModulation
{
public:
  static TypeId GetTypeId (void);
  SdrUcpModulation ();

  void SetConstellation (sdrucp::Constellation c);
  sdrucp::Constellation GetConstellation (void) const { return m_constellation; }

  // Bits per second = symbol rate * bits per symbol. Overrides the upstream
  // Bps() = m_sps/m_codingEff, which has no notion of constellation order.
  virtual double Bps (void) { return static_cast<double> (m_sps) * sdrucp::BitsPerSymbol (m_constellation); }

  // SNR-aware. snrLinear is Es/N0. Upstream Per(int) is left alone so nothing
  // that calls it through the base pointer changes behaviour unexpectedly.
  double PerAtSnr (double snrLinear, uint32_t pktBits) const
  { return sdrucp::Per (m_constellation, snrLinear, pktBits); }

  double BerAtSnr (double snrLinear) const
  { return sdrucp::Ber (m_constellation, snrLinear); }

private:
  sdrucp::Constellation m_constellation;
  uint32_t m_cIndex;   // attribute-facing copy (0=BPSK,1=QPSK,2=16QAM)
};
} // namespace ns3
#endif
