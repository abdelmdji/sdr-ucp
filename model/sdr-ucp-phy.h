#ifndef SDR_UCP_PHY_H
#define SDR_UCP_PHY_H
#include "ns3/aqua-sim-phy-cmn.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/sdr-ucp-ber.h"
#include <map>
namespace ns3 {

struct SnrObservation
{
  double snrDb;
  Time   stamp;
  SnrObservation () : snrDb (-100.0), stamp (Seconds (-1)) {}
};

class SdrUcpPhy : public AquaSimPhyCmn
{
public:
  static TypeId GetTypeId (void);
  SdrUcpPhy ();

  void SetActiveConstellation (sdrucp::Constellation c);
  sdrucp::Constellation GetActiveConstellation (void) const { return m_active; }

  bool GetNeighbourSnrDb (int neighbourAddr, Time maxAge, double &snrDbOut) const;

  enum AdaptMode { FIXED = 0, ADAPTIVE = 1 };
  enum CsiRule   { RULE_BEST = 0, RULE_MEDIAN = 1, RULE_WORST = 2 };

  sdrucp::Constellation ChooseConstellation (uint32_t pktBits) const;

  uint64_t GetSelectCount (sdrucp::Constellation c) const { return m_selected[c]; }
  uint64_t GetStaleFallbackCount (void) const { return m_staleFallback; }

  virtual bool PktTransmit (Ptr<Packet> p, int channelId = 0);
  int64_t AssignStreams (int64_t stream);

protected:
  virtual Ptr<Packet> PrevalidateIncomingPkt (Ptr<Packet> p);

private:
  sdrucp::Constellation m_active;
  double m_noiseFloorW;
  bool   m_applyPer;
  uint32_t m_adaptMode;
  uint32_t m_csiRule;
  double   m_targetPer;
  Time     m_maxCsiAge;
  mutable uint64_t m_selected[3];
  mutable uint64_t m_staleFallback;
  std::map<int, SnrObservation> m_snrTable;
  Ptr<UniformRandomVariable> m_rng;

  uint64_t m_detected;
  uint64_t m_perDropped;

public:
  uint64_t GetDetectedCount (void) const { return m_detected; }
  uint64_t GetPerDroppedCount (void) const { return m_perDropped; }
};
} // namespace ns3
#endif
