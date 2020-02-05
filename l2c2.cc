 #include "ns3/core-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/network-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/csma-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/yans-wifi-helper.h"
 #include "ns3/ssid.h"
 #include "ns3/netanim-module.h"
 #include "ns3/flow-monitor.h"
 #include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/on-off-helper.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include<stdlib.h>
#include<time.h>
 

using namespace ns3;
using namespace std;

int main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nWifi = 4;
 
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.Parse (argc, argv);

  if (nWifi > 18)
  {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
  }

  if (verbose)
  {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  }

  //1.create nodes   
  NodeContainer nodes;
  nodes.Create (nWifi);

  // 2. Create & setup wifi channel
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  // 3. Install wireless devices
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");
  WifiMacHelper mac;
  mac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices;
  devices = wifi.Install (phy, mac, nodes);
  MobilityHelper mobility;
  
  //4. Place nodes somehow
  
  // setup the grid itself: objects are laid out
  // started from (0,0) with 4 objects per row, 
  // the x interval between each object is 5 meters
  // and the y interval between each object is 5 meters
  
   /* mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (100.0),
                                 "DeltaY", DoubleValue (100.0),
                                 "GridWidth", UintegerValue (4),
                                 "LayoutType", StringValue ("RowFirst"));*/
  // each object will be attached a static position.
  // i.e., once set by the "position allocator", the
  // position will never change.

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (20.0, 0.0, 0.0));
  positionAlloc->Add (Vector (40.0, 0.0, 0.0));
  positionAlloc->Add (Vector (50.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
     
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (-200, 200, -200, 200)));
  
   //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  // finalize the setup by attaching to each object
  // in the input array a position and initializing
  // this position with the calculated coordinates.
  
  mobility.Install (nodes);

 /*mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|200|0|200"));
  mobility.InstallAll ();*/
  

  // 5. Install TCP/IP stack & assign IP addresses
  
  InternetStackHelper stack;
  stack.Install (nodes);
  
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  
  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces = address.Assign (devices);

  //6.

  UdpEchoServerHelper echoServer (9);

 ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (wifiInterfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (2));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (2048));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (2));
  clientApps.Start (Seconds (0.5));
  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // 8. Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  // 9. Run simulation for 10 seconds
  Simulator::Stop (Seconds (3));
  AnimationInterface anim("l2c2.xml");

  Simulator::Run ();

  // 10. Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      
      // Duration for throughput measurement is 9.0 seconds, since
      //   StartTime of the OnOffApplication is at about "second 1"
      // and
      //   Simulator::Stops at "second 10".
     //cout<<i->first<<endl; 
      if (i->first)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          cout << "  Tx Packets: " << i->second.txPackets << "\n";
          cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 2.0 / 1000 / 1000  << " Mbps\n";
          cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
          cout << "  Throughput: " << i->second.rxBytes * 8.0 / 2.0 / 1000 / 1000  << " Mbps\n";
          uint32_t lost= i->second.txPackets - i->second.rxPackets;
          cout << "  Packet Loss Ratio: " << lost*100 / i->second.txPackets << "%\n";
        }
    }

  // 11. Cleanup
  Simulator::Destroy ();
}
