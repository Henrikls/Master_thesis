#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-abilene-new.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.setCsSize(1500);
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
  Ptr<Node> routers[6] = {Names::Find<Node>("LOSAng"), Names::Find<Node>("DNVRng"), Names::Find<Node>("ATLAng"),
                            Names::Find<Node>("KSCYng"), Names::Find<Node>("IPLSng"), Names::Find<Node>("WASHng")};
                            
  Ptr<Node> producer = Names::Find<Node>("HSTNng");

  for (int i = 0; i < 4; i++) {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
    consumerHelper.SetAttribute("Frequency", StringValue("10")); // 100 interests a second

    // Each consumer will express the same data /root/<seq-no>
    consumerHelper.SetPrefix("/HSTNng");
    ApplicationContainer app = consumerHelper.Install(consumers[i]);
    app.Start(Seconds(0.01 * i));
  }

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

  // Register /root prefix with global routing controller and
  // install producer that will satisfy Interests in /root namespace
  ndnGlobalRoutingHelper.AddOrigins("/HSTNng", producer);
  producerHelper.SetPrefix("/HSTNng");
  producerHelper.Install(producer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(20.0));

  ndn::CsTracer::InstallAll("cs-trace-scen2-1500cs.txt", Seconds(1));
  ndn::L3RateTracer::InstallAll("rate-trace-scen2-1500cs.txt", Seconds(1));
  ndn::AppDelayTracer::InstallAll("app-delays-scen2-1500cs.txt");

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
