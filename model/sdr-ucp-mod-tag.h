#ifndef SDR_UCP_MOD_TAG_H
#define SDR_UCP_MOD_TAG_H
#include "ns3/tag.h"
#include <stdint.h>
namespace ns3 {
/* Carries the constellation chosen by the transmitter so the receiver can
 * demodulate with the matching BER curve.
 *
 * Why a Tag and not a header field: AquaSimPacketStamp in aqua-sim-header.h
 * has m_modName and its accessors present but COMMENTED OUT upstream, so the
 * PHY header cannot carry this. A Tag needs no upstream patch, survives the
 * header add/remove churn in PrevalidateIncomingPkt, and follows the pattern
 * Aqua-Sim already uses in aqua-sim-pt-tag.h.
 */
class SdrUcpModTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  SdrUcpModTag ();
  void SetConstellation (uint8_t c) { m_c = c; }
  uint8_t GetConstellation (void) const { return m_c; }
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;
private:
  uint8_t m_c;   // sdrucp::Constellation
};
} // namespace ns3
#endif
