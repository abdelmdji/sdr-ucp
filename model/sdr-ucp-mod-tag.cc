#include "ns3/sdr-ucp-mod-tag.h"
#include "ns3/sdr-ucp-ber.h"
namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (SdrUcpModTag);
SdrUcpModTag::SdrUcpModTag () : m_c (0) {}
TypeId
SdrUcpModTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdrUcpModTag")
    .SetParent<Tag> ()
    .AddConstructor<SdrUcpModTag> ();
  return tid;
}
TypeId SdrUcpModTag::GetInstanceTypeId (void) const { return GetTypeId (); }
uint32_t SdrUcpModTag::GetSerializedSize (void) const { return 1; }
void SdrUcpModTag::Serialize (TagBuffer i) const { i.WriteU8 (m_c); }
void SdrUcpModTag::Deserialize (TagBuffer i) { m_c = i.ReadU8 (); }
void
SdrUcpModTag::Print (std::ostream &os) const
{
  os << "mod=" << sdrucp::Name (static_cast<sdrucp::Constellation> (m_c));
}
} // namespace ns3
