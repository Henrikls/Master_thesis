/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * PrefetchingStrategy.hpp - Custom NDN forwarding strategy for prefetching
 *
 * This file defines the PrefetchingStrategy class, which extends the
 * BestRouteStrategy to include prefetching logic based on popularity or
 * engagement metrics.
 */

#ifndef NFD_DAEMON_FW_PREFETCHING_STRATEGY_HPP
#define NFD_DAEMON_FW_PREFETCHING_STRATEGY_HPP

#include "best-route-strategy.hpp" // Base class
#include <ndn-cxx/name.hpp>       // For NDN names
#include <ndn-cxx/name-component.hpp> // For name components
#include <nlohmann/json.hpp>      // For JSON processing
#include <vector>                 // For storing popular segments
#include <string>                 // For working with file paths

namespace nfd {
namespace fw {

class PrefetchingStrategy : public BestRouteStrategy
{
public:
  explicit PrefetchingStrategy(Forwarder& forwarder, const ::ndn::Name& name = getStrategyName());

  // Get the strategy's name
  static const ::ndn::Name& getStrategyName();

  // Override function to handle incoming interests
  void afterReceiveInterest(const ::ndn::Interest& interest, const FaceEndpoint& ingress,
                            const std::shared_ptr<pit::Entry>& pitEntry) override;

private:
  // Helper function to load popular segments from a JSON file
  void loadPopularSegments(const std::string& jsonFilePath);

  // Helper function to trigger prefetching
  void triggerPrefetch(const ::ndn::Name& segmentName, const std::shared_ptr<pit::Entry>& pitEntry);

  // List of popular segments for prefetching
  std::vector<int> popularSegments;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_PREFETCHING_STRATEGY_HPP

