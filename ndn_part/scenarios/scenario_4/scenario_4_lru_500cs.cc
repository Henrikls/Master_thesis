#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/names.h"

#include "/mnt/c/Users/Henri/Desktop/Mastersprojekt/ndnSIM/ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.hpp"
#include "/mnt/c/Users/Henri/Desktop/Mastersprojekt/ndnSIM/ns-3/src/ndnSIM/NFD/daemon/table/cs.hpp"
#include "/mnt/c/Users/Henri/Desktop/Mastersprojekt/ndnSIM/ns-3/src/ndnSIM/ndn-cxx/ndn-cxx/interest.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
//#include "ns3/ndnSIM/NFD/daemon/fw/forwarder.hpp"
//#include "ns3/ndnSIM/NFD/daemon/fw/PrefetchingStrategy.hpp"


#include <nlohmann/json.hpp>
#include <fstream>
#include <queue>
#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <cstddef>
#include <limits>

using namespace std;

namespace ns3 {

// Structure to store segment details
struct Segment {
    int id;
    std::string startTime;
    std::string endTime;
    float engagement;
};

// Global variables
std::set<std::string> prefetchedSegments;
std::map<Ptr<Node>, std::vector<Ptr<Node>>> routerToConsumersMap;

// Function to load and filter segments
void loadSegments(const std::string& jsonFilePath,
                  std::vector<Segment>& sequentialSegments,
                  std::vector<Segment>& popularSegments,
                  float threshold) {
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open JSON file");
    }

    nlohmann::json data;
    file >> data;

    for (const auto& segment : data) {
        Segment seg = {segment.at("Segment"),
                       segment.at("Start Time"),
                       segment.at("End Time"),
                       segment.at("Average Engagement")};
        sequentialSegments.push_back(seg);

        if (seg.engagement >= threshold) {
            popularSegments.push_back(seg);
        }
    }

    std::sort(sequentialSegments.begin(), sequentialSegments.end(),
              [](const Segment& a, const Segment& b) {
                  return a.startTime < b.startTime; // Ascending order
              });

    std::sort(popularSegments.rbegin(), popularSegments.rend(),
              [](const Segment& a, const Segment& b) {
                  return a.engagement < b.engagement; // Descending order
              });
}

// Function to check if two nodes are connected
bool areNodesConnected(Ptr<Node> nodeA, Ptr<Node> nodeB) {
    for (uint32_t i = 0; i < nodeA->GetNDevices(); i++) {
        Ptr<NetDevice> deviceA = nodeA->GetDevice(i);
        Ptr<Channel> channelA = deviceA->GetChannel();
        if (!channelA) continue;

        for (uint32_t j = 0; j < nodeB->GetNDevices(); j++) {
            Ptr<NetDevice> deviceB = nodeB->GetDevice(j);
            if (channelA == deviceB->GetChannel()) {
                return true; // Nodes are connected through this channel
            }
        }
    }
    return false;
}

// Function to build the router-to-consumer map
void buildRouterToConsumersMap(const std::vector<Ptr<Node>>& routers, const std::vector<Ptr<Node>>& consumers) {
    for (auto router : routers) {
        for (auto consumer : consumers) {
            if (areNodesConnected(router, consumer)) {
                routerToConsumersMap[router].push_back(consumer);
            }
        }
    }
}

// Function to calculate hop count using BFS
int calculateHopCount(Ptr<Node> src, Ptr<Node> dst) {
    std::queue<std::pair<Ptr<Node>, int>> queue;
    std::set<Ptr<Node>> visited;
    queue.push({src, 0});
    visited.insert(src);

    while (!queue.empty()) {
        auto [current, depth] = queue.front();
        queue.pop();

        if (current == dst) {
            return depth;
        }

        for (uint32_t i = 0; i < current->GetNDevices(); i++) {
            Ptr<NetDevice> device = current->GetDevice(i);
            Ptr<Channel> channel = device->GetChannel();
            if (!channel) continue;

            for (uint32_t j = 0; j < channel->GetNDevices(); j++) {
                Ptr<NetDevice> neighborDevice = channel->GetDevice(j);
                Ptr<Node> neighbor = neighborDevice->GetNode();
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    queue.push({neighbor, depth + 1});
                }
            }
        }
    }

    return std::numeric_limits<int>::max(); // Return a large value if no path is found
}

// Replace `CalculateHopCount` with `calculateHopCount`
double calculateProximity(Ptr<Node> router, Ptr<Node> consumerNode) {
    return calculateHopCount(consumerNode, router);
}

double calculateRouterScore(Ptr<Node> router, Ptr<Node> consumerNode) {
    double proximityScore = calculateProximity(router, consumerNode);
    int connectedConsumers = routerToConsumersMap[router].size();
    double proximityWeight = 1.0;  // Adjust based on preference
    double consumerWeight = 1.5;  // Higher weight for consumer density
    return proximityWeight * proximityScore - consumerWeight * connectedConsumers;
}

// Function to find the nearest router
Ptr<Node> findNearestRouter(Ptr<Node> consumerNode, const std::vector<Ptr<Node>>& routers) {
    double bestScore = std::numeric_limits<double>::max();
    Ptr<Node> bestRouter = nullptr;

    for (const auto& router : routers) {
        double score = calculateRouterScore(router, consumerNode);
        if (score < bestScore) {
            bestScore = score;
            bestRouter = router;
        }
    }

    return bestRouter;
}

// Function to check if a segment is already cached
bool isSegmentCached(Ptr<Node> node, const std::string& prefix) {
    // Access the ndn::L3Protocol from the node
    auto l3Protocol = node->GetObject<ndn::L3Protocol>();
    if (!l3Protocol) {
        std::cout << "Error: L3Protocol not found on node " << node->GetId() << std::endl;
        return false;
    }

    // Access the shared_ptr to the Forwarder
    auto forwarderPtr = l3Protocol->getForwarder();
    if (!forwarderPtr) {
        std::cout << "Error: Forwarder not found on node " << node->GetId() << std::endl;
        return false;
    }

    // Dereference the shared_ptr to get the Forwarder object
    auto& forwarder = *forwarderPtr;

    // Access the Content Store
    auto& contentStore = forwarder.getCs();

    // Create a Name object from the prefix
    ndn::Name segmentName(prefix);

    // Iterate over all entries in the content store
    for (auto it = contentStore.begin(); it != contentStore.end(); ++it) {
        if (it->getName() == segmentName) {
            return true; // Segment is found in the cache
        }
    }

    return false; // Segment is not found in the cache
}

void handleConsumerBehavior(Ptr<Node> consumer, const std::string& behaviorType, 
                            const std::vector<Segment>& sequentialSegments, 
                            const std::vector<Segment>& popularSegments, 
                            const std::string& prefix, 
                            double& currentTime, std::ofstream& logFile) {
    if (behaviorType == "SEQUENTIAL") {
        for (const auto& segment : sequentialSegments) {
            std::string fetchPrefix = prefix + "/segment_" + std::to_string(segment.id) + ".mp4";
            if (!isSegmentCached(consumer, fetchPrefix)) {
                ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
                consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 interests/sec
                consumerHelper.SetPrefix(fetchPrefix);

                ApplicationContainer fetchApp = consumerHelper.Install(consumer);
                fetchApp.Start(Seconds(currentTime));

                logFile << "FETCH (SEQUENTIAL): Consumer: " << consumer->GetId()
                        << ", Segment: " << fetchPrefix
                        << ", Time: " << std::fixed << std::setprecision(2) << currentTime << " seconds\n";

                currentTime += 0.01;
            }
        }
    } else if (behaviorType == "SKIPPER") {
        for (size_t i = 0; i < sequentialSegments.size(); i += 2) { // Skip every other segment
            const auto& segment = sequentialSegments[i];
            std::string fetchPrefix = prefix + "/segment_" + std::to_string(segment.id) + ".mp4";
            if (!isSegmentCached(consumer, fetchPrefix)) {
                ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
                consumerHelper.SetAttribute("Frequency", StringValue("10"));
                consumerHelper.SetPrefix(fetchPrefix);

                ApplicationContainer fetchApp = consumerHelper.Install(consumer);
                fetchApp.Start(Seconds(currentTime));

                logFile << "FETCH (SKIPPER): Consumer: " << consumer->GetId()
                        << ", Segment: " << fetchPrefix
                        << ", Time: " << std::fixed << std::setprecision(2) << currentTime << " seconds\n";

                currentTime += 0.01;
            }
        }
    } else if (behaviorType == "POPULAR_ONLY") {
        for (const auto& segment : popularSegments) {
            std::string fetchPrefix = prefix + "/segment_" + std::to_string(segment.id) + ".mp4";
            if (!isSegmentCached(consumer, fetchPrefix)) {
                ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
                consumerHelper.SetAttribute("Frequency", StringValue("10"));
                consumerHelper.SetPrefix(fetchPrefix);

                ApplicationContainer fetchApp = consumerHelper.Install(consumer);
                fetchApp.Start(Seconds(currentTime));

                logFile << "FETCH (POPULAR_ONLY): Consumer: " << consumer->GetId()
                        << ", Segment: " << fetchPrefix
                        << ", Time: " << std::fixed << std::setprecision(2) << currentTime << " seconds\n";

                currentTime += 0.01;
            }
        }
    } else if (behaviorType == "RANDOM") {
        std::vector<Segment> allSegments = sequentialSegments;
        std::random_shuffle(allSegments.begin(), allSegments.end());

        for (const auto& segment : allSegments) {
            std::string fetchPrefix = prefix + "/segment_" + std::to_string(segment.id) + ".mp4";
            if (!isSegmentCached(consumer, fetchPrefix)) {
                ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
                consumerHelper.SetAttribute("Frequency", StringValue("10"));
                consumerHelper.SetPrefix(fetchPrefix);

                ApplicationContainer fetchApp = consumerHelper.Install(consumer);
                fetchApp.Start(Seconds(currentTime));

                logFile << "FETCH (RANDOM): Consumer: " << consumer->GetId()
                        << ", Segment: " << fetchPrefix
                        << ", Time: " << std::fixed << std::setprecision(2) << currentTime << " seconds\n";

                currentTime += 0.01;
            }
        }
    } else {
        logFile << "ERROR: Unknown behavior type: " << behaviorType << "\n";
    }
}

// Main function for the simulation
int main(int argc, char* argv[]) {
    CommandLine cmd;
    std::string prefix = "/mnt/c/Users/Henri/Desktop/Mastersprojekt/data_gather/Segmented_vid";
    std::string jsonFilePath = "/mnt/c/Users/Henri/Desktop/Mastersprojekt/data_gather/csv/segment_popularity.json";
    std::string logFilePath = "/mnt/c/Users/Henri/Desktop/Mastersprojekt/data_gather/csv/prefetch-log.txt";
    cmd.AddValue("jsonFile", "Path to the JSON file containing segment popularity data", jsonFilePath);
    cmd.AddValue("logFile", "Path to the log file for fetch/prefetch events", logFilePath);
    cmd.Parse(argc, argv);

    // Load segments
    std::vector<Segment> sequentialSegments, popularSegments;
    const float engagementThreshold = 50.0f;
    loadSegments(jsonFilePath, sequentialSegments, popularSegments, engagementThreshold);

    // Read topology
    AnnotatedTopologyReader topologyReader("", 25);
    topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-abilene-new.txt");
    topologyReader.Read();

    // Install NDN stack
    ndn::StackHelper ndnHelper;
    ndnHelper.setPolicy("nfd::cs::lru");
    ndnHelper.setCsSize(500);
    ndnHelper.InstallAll();

    // Set up forwarding strategy
    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

    // Set up consumers, routers, and producer
    std::vector<Ptr<Node>> consumers = {Names::Find<Node>("NYCMng"), Names::Find<Node>("CHINng"),
                                        Names::Find<Node>("STTLng"), Names::Find<Node>("SNVAng")};
     //Setting up the routers
    std::vector<Ptr<Node>> routers = {Names::Find<Node>("LOSAng"), Names::Find<Node>("DNVRng"), Names::Find<Node>("ATLAng"),
                                        Names::Find<Node>("KSCYng"), Names::Find<Node>("IPLSng"), Names::Find<Node>("WASHng")};
    
    Ptr<Node> producer = Names::Find<Node>("HSTNng");

    // Build router-to-consumer map
    buildRouterToConsumersMap(routers, consumers);

    // Log file for prefetch and fetch events
    std::ofstream logFile(logFilePath, std::ios_base::trunc);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file for writing." << std::endl;
        return -1;
    }

    // Prefetch popular segments
    double prefetchStartTime = 0.0;
    for (const auto& segment : popularSegments) {
        for (const auto& consumer : consumers) {
            Ptr<Node> bestRouter = findNearestRouter(consumer, routers);
            if (bestRouter) {
                std::string prefetchPrefix = prefix + "/segment_" + std::to_string(segment.id) + ".mp4";

                ndn::AppHelper prefetchHelper("ns3::ndn::ConsumerCbr");
                prefetchHelper.SetAttribute("Frequency", StringValue("5"));
                prefetchHelper.SetPrefix(prefetchPrefix);

                ApplicationContainer prefetchApp = prefetchHelper.Install(bestRouter);
                prefetchApp.Start(Seconds(prefetchStartTime));

                logFile << "PREFETCH: Router: " << bestRouter->GetId()
                        << ", Segment: " << prefetchPrefix
                        << ", Engagement: " << segment.engagement
                        << ", Time: " << prefetchStartTime << " seconds\n";
            }
        }
        prefetchStartTime += 0.01;
    }
    for (const auto& consumer : consumers) {
        Ptr<Node> bestRouter = findNearestRouter(consumer, routers);
    }

    // Fetch behavior setup
    double fetchStartTime = 1.0;
    std::vector<std::string> consumerBehaviors = {"SEQUENTIAL", "SKIPPER", "POPULAR_ONLY", "RANDOM"};
    for (size_t i = 0; i < consumers.size(); i++) {
        handleConsumerBehavior(consumers[i], consumerBehaviors[i], sequentialSegments, popularSegments, prefix, fetchStartTime, logFile);
    }

    logFile.close();

    // Producer setup
    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetPrefix(prefix);
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.Install(producer);

    // Set up global routing
    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
    ndnGlobalRoutingHelper.InstallAll();
    ndnGlobalRoutingHelper.AddOrigins(prefix, producer);
    ndnGlobalRoutingHelper.CalculateRoutes();

    // Enable tracing
    ndn::CsTracer::InstallAll("cs-trace-scen4-lru-500cs.txt", Seconds(1));
    ndn::L3RateTracer::InstallAll("rate-trace-scen4-lru-500cs.txt", Seconds(1));
    ndn::AppDelayTracer::InstallAll("app-delays-trace-scen4-lru-500cs.txt");

    // Run the simulation
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}

} // namespace ns3

int main(int argc, char* argv[]) {
    return ns3::main(argc, argv);
}
