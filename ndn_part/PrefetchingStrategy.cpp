#include "PrefetchingStrategy.hpp"
#include "algorithm.hpp"
#include "common/logger.hpp"

#include "NFD/daemon/table/cs.hpp"
#include "NFD/daemon/table/pit.hpp"
#include "NFD/daemon/face/face.hpp"
#include "NFD/daemon/table/fib-entry.hpp"
#include "NFD/daemon/table/fib.hpp"
#include "NFD/daemon/fw/forwarder.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

namespace nfd {
namespace fw {

NFD_LOG_INIT(PrefetchingStrategy);
NFD_REGISTER_STRATEGY(PrefetchingStrategy);

const Name& PrefetchingStrategy::getStrategyName() {
  static const auto strategyName = Name("/localhost/nfd/strategy/prefetching").appendVersion(1);
  //static const Name strategyName("/localhost/nfd/strategy/prefetching/1");
  NFD_LOG_DEBUG("getStrategyName() returns: " << strategyName);
  return strategyName;
}

PrefetchingStrategy::PrefetchingStrategy(Forwarder& forwarder, const Name& name)
    : BestRouteStrategy(forwarder, name) {
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("PrefetchingStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument(
      "PrefetchingStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
  NFD_LOG_DEBUG("PrefetchingStrategy initialized with instance name: " << this->getInstanceName());
  loadPopularSegments("/path/to/json");
}


void PrefetchingStrategy::loadPopularSegments(const std::string& jsonFilePath) {
  std::ifstream file(jsonFilePath);
  if (!file.is_open()) {
    NFD_LOG_ERROR("Unable to open JSON file for popular segments");
    return;
  }

  nlohmann::json data;
  file >> data;

  for (const auto& segment : data) {
    popularSegments.push_back(segment["Segment"]);
  }
}

void PrefetchingStrategy::afterReceiveInterest(const ::ndn::Interest& interest,
                                               const FaceEndpoint& ingress,
                                               const std::shared_ptr<pit::Entry>& pitEntry) {
  BestRouteStrategy::afterReceiveInterest(interest, ingress, pitEntry);

  const ::ndn::Name& currentInterest = interest.getName();
  ::ndn::Name videoPrefix = currentInterest.getPrefix(-1);

  for (const auto& segmentId : popularSegments) {
    ::ndn::Name prefetchSegment = videoPrefix;
    prefetchSegment.append("segment_" + std::to_string(segmentId));
    NFD_LOG_DEBUG("Prefetching segment: " << prefetchSegment);
    triggerPrefetch(prefetchSegment, pitEntry);
  }
}

void PrefetchingStrategy::triggerPrefetch(const ::ndn::Name& segmentName,
                                          const std::shared_ptr<pit::Entry>& pitEntry) {
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  ::ndn::Interest prefetchInterest(segmentName);
  prefetchInterest.setCanBePrefix(false);
  prefetchInterest.setInterestLifetime(::ndn::time::seconds(1));

  for (const auto& nexthop : nexthops) {
    const nfd::face::Face& face = nexthop.getFace();
    if (isNextHopEligible(face, prefetchInterest, nexthop, pitEntry)) {
      this->sendInterest(prefetchInterest, const_cast<nfd::face::Face&>(face), pitEntry);
      NFD_LOG_DEBUG("Prefetch triggered for segment: " << segmentName << " via Face: " << face.getId());
      return;
    }
  }

  NFD_LOG_DEBUG("No eligible next-hop for prefetching: " << segmentName);
}

} // namespace fw
} // namespace nfd
