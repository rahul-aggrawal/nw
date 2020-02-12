#include <sstream>
 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/config-store-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/dsr-module.h"
 #include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/on-off-helper.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
 
 using namespace ns3;
using namespace std;
 
 NS_LOG_COMPONENT_DEFINE ("DsrTest");
 
 int
 main (int argc, char *argv[])
 {
   //
   // Users may find it convenient to turn on explicit debugging
   // for selected modules; the below lines suggest how to do this
   //
 #if 0
   LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
   LogComponentEnable ("UdpL4Protocol", LOG_LEVEL_ALL);
   LogComponentEnable ("UdpSocketImpl", LOG_LEVEL_ALL);
   LogComponentEnable ("NetDevice", LOG_LEVEL_ALL);
   LogComponentEnable ("Ipv4EndPointDemux", LOG_LEVEL_ALL);
 #endif
 
 #if 0
   LogComponentEnable ("DsrOptions", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrHelper", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrRouting", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrOptionHeader", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrFsHeader", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrGraReplyTable", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrSendBuffer", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrRouteCache", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrMaintainBuffer", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrRreqTable", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrErrorBuffer", LOG_LEVEL_ALL);
   LogComponentEnable ("DsrNetworkQueue", LOG_LEVEL_ALL);
 #endif
 
   NS_LOG_INFO ("creating the nodes");
 
   // General parameters
   uint32_t nWifis = 50;
   uint32_t nSinks = 10;
   double TotalTime = 6.0;
   double dataTime = 5.0;
   double ppers = 1;
   uint32_t packetSize = 64;
   double dataStart = 1.0; // start sending data at 100s
 
   //mobility parameters
   double pauseTime = 0.0;
   double nodeSpeed = 20.0;
   double txpDistance = 250.0;
 
   std::string rate = "1024bps";
   std::string dataMode ("DsssRate11Mbps");
   std::string phyMode ("DsssRate11Mbps");
 
   //Allow users to override the default parameters and set it to new ones from CommandLine.
   CommandLine cmd;
   cmd.AddValue ("nWifis", "Number of wifi nodes", nWifis);
   cmd.AddValue ("nSinks", "Number of SINK traffic nodes", nSinks);
   cmd.AddValue ("rate", "CBR traffic rate(in kbps), Default:8", rate);
   cmd.AddValue ("nodeSpeed", "Node speed in RandomWayPoint model, Default:20", nodeSpeed);
   cmd.AddValue ("packetSize", "The packet size", packetSize);
   cmd.AddValue ("txpDistance", "Specify node's transmit range, Default:300", txpDistance);
   cmd.AddValue ("pauseTime", "pauseTime for mobility model, Default: 0", pauseTime);
   cmd.Parse (argc, argv);
 
   SeedManager::SetSeed (10);
   SeedManager::SetRun (1);
 
   NodeContainer adhocNodes;
   adhocNodes.Create (nWifis);
   NetDeviceContainer allDevices;
 
   NS_LOG_INFO ("setting the default phy and channel parameters");
   Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));
   Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
   // disable fragmentation for frames below 2200 bytes
   Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
 
   NS_LOG_INFO ("setting the default phy and channel parameters ");
   WifiHelper wifi;
   wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
   YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
 
   YansWifiChannelHelper wifiChannel;
   wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (txpDistance));
   wifiPhy.SetChannel (wifiChannel.Create ());
 
   // Add a mac and disable rate control
   WifiMacHelper wifiMac;
   wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (dataMode), "ControlMode",
                                 StringValue (phyMode));
 
   wifiMac.SetType ("ns3::AdhocWifiMac");
   allDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);
 
   NS_LOG_INFO ("Configure Tracing.");
 
   AsciiTraceHelper ascii;
   Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("dsrtest.tr");
   wifiPhy.EnableAsciiAll (stream);
 
   MobilityHelper adhocMobility;
   ObjectFactory pos;
   pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
   pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
   pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));
   Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
 
   std::ostringstream speedUniformRandomVariableStream;
   speedUniformRandomVariableStream << "ns3::UniformRandomVariable[Min=0.0|Max="
                                    << nodeSpeed
                                    << "]";
 
   std::ostringstream pauseConstantRandomVariableStream;
   pauseConstantRandomVariableStream << "ns3::ConstantRandomVariable[Constant="
                                     << pauseTime
                                     << "]";
 
   adhocMobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                   //                                  "Speed", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=nodeSpeed]"),
                                   "Speed", StringValue (speedUniformRandomVariableStream.str ()),
                                   "Pause", StringValue (pauseConstantRandomVariableStream.str ()),
                                   "PositionAllocator", PointerValue (taPositionAlloc)
                                   );
   adhocMobility.Install (adhocNodes);
 
   InternetStackHelper internet;
   DsrMainHelper dsrMain;
   DsrHelper dsr;
   internet.Install (adhocNodes);
   dsrMain.Install (dsr, adhocNodes);
 
   NS_LOG_INFO ("assigning ip address");
   Ipv4AddressHelper address;
   address.SetBase ("10.1.1.0", "255.255.255.0");
   Ipv4InterfaceContainer allInterfaces;
   allInterfaces = address.Assign (allDevices);
 
   uint16_t port = 9;
   double randomStartTime = (1 / ppers) / nSinks; //distributed btw 1s evenly as we are sending 4pkt/s
 
   for (uint32_t i = 0; i < nSinks; ++i)
     {
       PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
       ApplicationContainer apps_sink = sink.Install (adhocNodes.Get (i));
       apps_sink.Start (Seconds (0.0));
       apps_sink.Stop (Seconds (TotalTime));
 
       OnOffHelper onoff1 ("ns3::UdpSocketFactory", Address (InetSocketAddress (allInterfaces.GetAddress (i), port)));
       onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
       onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
       onoff1.SetAttribute ("PacketSize", UintegerValue (packetSize));
       onoff1.SetAttribute ("DataRate", DataRateValue (DataRate (rate)));
 
       ApplicationContainer apps1 = onoff1.Install (adhocNodes.Get (i + nWifis - nSinks));
       apps1.Start (Seconds (dataStart + i * randomStartTime));
       apps1.Stop (Seconds (dataTime + i * randomStartTime));
     }
 
   // 8. Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

	// 9. Run simulation for 10 seconds
  Simulator::Stop (Seconds (TotalTime));

  AnimationInterface anim("netdsr.xml");

  Simulator::Run ();
	
	// 10. Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      if (i->first)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          cout << "  Tx Packets: " << i->second.txPackets << "\n";
          cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000   << " Kbps\n";
          cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
          cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000   << " Kbps\n";
          uint32_t lost= i->second.txPackets - i->second.rxPackets;
          cout << "  Packet Loss Ratio: " << lost*100 / i->second.txPackets << "%\n";
        }
    }

  // 11. Cleanup
  Simulator::Destroy ();
 }
