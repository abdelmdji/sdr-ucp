#include "ns3/sdr-ucp-phy.h"
#include "ns3/sdr-ucp-modulation.h"
#include "ns3/sdr-ucp-mod-tag.h"
#include "ns3/aqua-sim-header.h"
#include "ns3/aqua-sim-net-device.h"
#include "ns3/aqua-sim-signal-cache.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <algorithm>
#include <vector>

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("SdrUcpPhy");
NS_OBJECT_ENSURE_REGISTERED (SdrUcpPhy);

SdrUcpPhy::SdrUcpPhy ()
  : m_active (sdrucp::BPSK), m_noiseFloorW (1e-12), m_applyPer (true),
    m_adaptMode (FIXED), m_csiRule (RULE_BEST), m_targetPer (0.10),
    m_maxCsiAge (Seconds (30)), m_staleFallback (0),
    m_detected (0), m_perDropped (0)
{
  m_rng = CreateObject<UniformRandomVariable> ();
  m_selected[0] = m_selected[1] = m_selected[2] = 0;
}

TypeId
SdrUcpPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdrUcpPhy")
    .SetParent<AquaSimPhyCmn> ()
    .AddConstructor<SdrUcpPhy> ()
    .AddAttribute ("NoiseFloor", "Ambient noise power (W).",
                   DoubleValue (1e-12),
                   MakeDoubleAccessor (&SdrUcpPhy::m_noiseFloorW),
                   MakeDoubleChecker<double> (0.0))
    .AddAttribute ("ApplyPer", "Apply modulation-dependent packet error model.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SdrUcpPhy::m_applyPer),
                   MakeBooleanChecker ())
    .AddAttribute ("AdaptMode", "0 = FIXED, 1 = ADAPTIVE.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SdrUcpPhy::m_adaptMode),
                   MakeUintegerChecker<uint32_t> (0, 1))
    .AddAttribute ("CsiRule", "0 = best, 1 = median, 2 = worst neighbour SNR.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SdrUcpPhy::m_csiRule),
                   MakeUintegerChecker<uint32_t> (0, 2))
    .AddAttribute ("TargetPer", "Highest packet error rate the controller accepts.",
                   DoubleValue (0.10),
                   MakeDoubleAccessor (&SdrUcpPhy::m_targetPer),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("MaxCsiAge", "Observations older than this are unusable.",
                   TimeValue (Seconds (30)),
                   MakeTimeAccessor (&SdrUcpPhy::m_maxCsiAge),
                   MakeTimeChecker ());
  return tid;
}

int64_t
SdrUcpPhy::AssignStreams (int64_t stream)
{
  m_rng->SetStream (stream);
  return 1;
}

void
SdrUcpPhy::SetActiveConstellation (sdrucp::Constellation c)
{
  m_active = c;
  m_modulationName = std::string (sdrucp::Name (c));
}

bool
SdrUcpPhy::GetNeighbourSnrDb (int neighbourAddr, Time maxAge, double &snrDbOut) const
{
  std::map<int, SnrObservation>::const_iterator it = m_snrTable.find (neighbourAddr);
  if (it == m_snrTable.end ())        return false;
  if (it->second.stamp < Seconds (0)) return false;
  if (Simulator::Now () - it->second.stamp > maxAge) return false;
  snrDbOut = it->second.snrDb;
  return true;
}

sdrucp::Constellation
SdrUcpPhy::ChooseConstellation (uint32_t pktBits) const
{
  std::vector<double> fresh;
  const Time now = Simulator::Now ();
  for (std::map<int, SnrObservation>::const_iterator it = m_snrTable.begin ();
       it != m_snrTable.end (); ++it)
    {
      if (it->second.stamp < Seconds (0)) continue;
      if (now - it->second.stamp > m_maxCsiAge) continue;
      fresh.push_back (it->second.snrDb);
    }

  if (fresh.empty ())
    {
      ++m_staleFallback;
      ++m_selected[sdrucp::BPSK];
      return sdrucp::BPSK;
    }

  std::sort (fresh.begin (), fresh.end ());
  double snrDb;
  switch (m_csiRule)
    {
    case RULE_WORST:  snrDb = fresh.front (); break;
    case RULE_MEDIAN: snrDb = fresh[fresh.size () / 2]; break;
    case RULE_BEST:
    default:          snrDb = fresh.back (); break;
    }

  const double snrLin = sdrucp::DbToLinear (snrDb);
  sdrucp::Constellation pick = sdrucp::BPSK;
  if (sdrucp::Per (sdrucp::QAM16, snrLin, pktBits) <= m_targetPer)
    pick = sdrucp::QAM16;
  else if (sdrucp::Per (sdrucp::QPSK, snrLin, pktBits) <= m_targetPer)
    pick = sdrucp::QPSK;

  ++m_selected[pick];
  NS_LOG_DEBUG ("adapt: n=" << fresh.size () << " snr=" << snrDb
                << "dB -> " << sdrucp::Name (pick));
  return pick;
}

bool
SdrUcpPhy::PktTransmit (Ptr<Packet> p, int channelId)
{
  if (m_adaptMode == ADAPTIVE)
    {
      AquaSimHeader ash;
      p->PeekHeader (ash);
      SetActiveConstellation (ChooseConstellation (ash.GetSize () * 8));
    }

  SdrUcpModTag tag;
  tag.SetConstellation (static_cast<uint8_t> (m_active));
  p->ReplacePacketTag (tag);
  return AquaSimPhyCmn::PktTransmit (p, channelId);
}

Ptr<Packet>
SdrUcpPhy::PrevalidateIncomingPkt (Ptr<Packet> p)
{
  AquaSimPacketStamp stamp;
  AquaSimHeader ash;
  p->RemoveHeader (stamp);
  p->PeekHeader (ash);

  const double pr = stamp.GetPr ();

  double noise = m_noiseFloorW;
  if (GetSignalCache ())
    {
      const double total = GetSignalCache ()->GetNoise ();
      const double interference = total - pr;
      noise = (interference > 0.0 ? interference : 0.0) + m_noiseFloorW;
    }
  const double snrLinear = (noise > 0.0) ? (pr / noise) : 0.0;

  bool corrupt = false;

  if (pr >= GetCSThresh ())
    {
      ++m_detected;
      SnrObservation obs;
      obs.snrDb = sdrucp::LinearToDb (snrLinear > 0.0 ? snrLinear : 1e-30);
      obs.stamp = Simulator::Now ();
      m_snrTable[ash.GetSAddr ().GetAsInt ()] = obs;

      if (m_applyPer)
        {
          SdrUcpModTag tag;
          sdrucp::Constellation c = sdrucp::BPSK;
          if (p->PeekPacketTag (tag))
            c = static_cast<sdrucp::Constellation> (tag.GetConstellation ());
          else
            NS_LOG_WARN ("no SdrUcpModTag on packet; assuming BPSK");

          const uint32_t bits = ash.GetSize () * 8;
          const double per = sdrucp::Per (c, snrLinear, bits);
          if (m_rng->GetValue (0.0, 1.0) < per)
            {
              corrupt = true;
              ++m_perDropped;
              NS_LOG_DEBUG ("PER drop: mod=" << sdrucp::Name (c)
                            << " snr=" << obs.snrDb << "dB per=" << per);
            }
        }
    }

  if (corrupt)
    {
      p->RemoveHeader (ash);
      ash.SetErrorFlag (true);
      p->AddHeader (ash);
    }

  p->AddHeader (stamp);
  return AquaSimPhyCmn::PrevalidateIncomingPkt (p);
}
} // namespace ns3
