#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  //used to take logs
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  //creating n nodes       
  NodeContainer nodes;
  nodes.Create (2);
  //choose channel to communicate
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); //setting device attribute
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms")); //setting channel attribute
  //install channel on nodes
  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);
  //ask nodes to follow rules
  InternetStackHelper stack;
  stack.Install (nodes);
  //assign ip address to nodes
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);
  //create a x type of server on port x
  UdpEchoServerHelper echoServer (9);
  //install server on a node then start and stop the server
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0));//server starts at time=1
  serverApps.Stop (Seconds (10.0));
  //create x type of client and set its attributes
  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  //install client on a node then start and stop the client
  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));//client starts at time=2
  clientApps.Stop (Seconds (10.0));
  //Run the simulation
	AnimationInterface anim("first.xml");
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
