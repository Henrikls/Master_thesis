#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  std::string prefix1 = "IPLSng/Segmented_vid";
  std::string prefix2 = "DNVRng/Segmented_vid";
  cmd.Parse(argc, argv);
  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-abilene-new.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.setCsSize(0);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  
  //Setting up the consumers
  Ptr<Node> consumers[4] = {Names::Find<Node>("NYCMng"), Names::Find<Node>("CHINng"),
                            Names::Find<Node>("STTLng"), Names::Find<Node>("SNVAng")};

  //Setting up the routers
  Ptr<Node> routers[4] = {Names::Find<Node>("LOSAng"), Names::Find<Node>("ATLAng"),
                            Names::Find<Node>("KSCYng"), Names::Find<Node>("WASHng")};
                            
  Ptr<Node> producer[2] = {Names::Find<Node>("IPLSng"), Names::Find<Node>("DNVRng")};

  // Configure consumers to fetch 14 segments
  double startTime = 1.0;

  for (int segment = 1; segment <= 14; segment++) {
    std::string segmentPrefix1 = prefix1 + "/segment_" + std::to_string(segment) + ".mp4";
    std::string segmentPrefix2 = prefix2 + "/segment_" + std::to_string(segment) + ".mp4";

    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
    consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 Interests/sec

    for (int i = 0; i < 2; i++) {
      consumerHelper.SetPrefix(segmentPrefix1);
      ApplicationContainer app = consumerHelper.Install(consumers[i]);
      app.Start(Seconds(startTime));
    }

    for (int i = 2; i < 4; i++) {
      consumerHelper.SetPrefix(segmentPrefix2);
      ApplicationContainer app = consumerHelper.Install(consumers[i]);
      app.Start(Seconds(startTime));
    }

    startTime += 0.01; // Increment time for each segment
  }

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

  // Producer1 setup
  producerHelper.SetPrefix(prefix1);
  producerHelper.Install(producer[0]);

  // Producer2 setup
  producerHelper.SetPrefix(prefix2);
  producerHelper.Install(producer[1]);

  // Set up global routing for producer1
  ndnGlobalRoutingHelper.AddOrigins(prefix1, producer[0]);

  // Set up global routing for producer2
  ndnGlobalRoutingHelper.AddOrigins(prefix2, producer[1]);
  
  ndnGlobalRoutingHelper.CalculateRoutes();

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(20.0));

  ndn::CsTracer::InstallAll("cs-trace-scen1-0cs.txt", Seconds(1));
  ndn::L3RateTracer::InstallAll("rate-trace-scen1-0cs.txt", Seconds(1));
  ndn::AppDelayTracer::InstallAll("app-delays-scen1-0cs.txt");

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
