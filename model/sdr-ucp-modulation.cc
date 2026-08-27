#include "ns3/sdr-ucp-modulation.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("SdrUcpModulation");
NS_OBJECT_ENSURE_REGISTERED (SdrUcpModulation);

SdrUcpModulation::SdrUcpModulation () : m_constellation (sdrucp::BPSK), m_cIndex (0) {}

TypeId
SdrUcpModulation::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdrUcpModulation")
    .SetParent<AquaSimModulation> ()
    .AddConstructor<SdrUcpModulation> ()
    .AddAttribute ("Constellation",
                   "0 = BPSK, 1 = QPSK, 2 = 16-QAM.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SdrUcpModulation::m_cIndex),
                   MakeUintegerChecker<uint32_t> (0, 2));
  return tid;
}

void
SdrUcpModulation::SetConstellation (sdrucp::Constellation c)
{
  m_constellation = c;
  m_cIndex = static_cast<uint32_t> (c);
  NS_LOG_DEBUG ("constellation set to " << sdrucp::Name (c)
                << " bps=" << Bps ());
}
} // namespace ns3
