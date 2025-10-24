/*
 * Lab 04 — BATCH LTE Downlink End-to-End Experiment Driver (Corrected)
 * ----------------------------------------------------------------
 * What this program does:
 *   - Runs a series of LTE downlink simulations by sweeping through a
 *     predefined set of parameters (antenna, data rate, distance).
 *   - Encapsulates each simulation run into a reusable function, ensuring
 *     a clean state for each experiment without restarting the process.
 *   - Aggregates all results into a single CSV file for easy analysis
 *     and plotting.
 *   - Centralizes key simulation parameters for easy tuning.
 *
 * How to compile & run:
 *   1. Place this file in your ns-3 'scratch/' directory.
 *   2. Build it once:
 *      ./ns3 build
 *   3. Run the entire batch experiment:
 *      ./ns3 run scratch/Lab4_Cpp_LTE-Batch
 *
 * Output:
 *   - A single CSV file named 'results/all_experiments.csv' containing
 *     the throughput data for every parameter combination.
 *   - NOTE: Per-run trace files (PDCP, RLC, PCAP) will overwrite each other
 *     with each new experiment run.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/lte-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include <fstream>
#include <string>
#include <vector>

using namespace ns3;

// =================================================================
// ============== 1. PARAMETER TUNING CONSTANTS ====================
// =================================================================
// Easily modify these core simulation settings for all batch runs.
const uint32_t    DL_BANDWIDTH_RBS = 50;         // Downlink bandwidth in Resource Blocks (50 = 10 MHz)
const uint32_t    UL_BANDWIDTH_RBS = 50;         // Uplink bandwidth in Resource Blocks (50 = 10 MHz)
const uint32_t    DL_EARFCN        = 100;        // Carrier frequency
const uint32_t    UL_EARFCN        = 18100;      // Carrier frequency
const std::string SCHEDULER        = "ns3::PfFfMacScheduler";
const std::string PATHLOSS_MODEL   = "ns3::FriisPropagationLossModel"; // Or TwoRayGround, etc.
const std::string CSV_OUTPUT_PATH  = "results/all_experiments.csv";

// =================================================================
// ============== 2. DATA STRUCTURES FOR EXPERIMENTS ===============
// =================================================================

// Holds all input parameters for a single simulation run.
struct RunConfig
{
    std::string antenna;
    double      distance;
    std::string dataRate;
    double      enbOrient;
    double      ueOrient;
    uint32_t    seed;
};

// Holds the results from a single simulation run.
struct RunResult
{
    uint64_t rxBytes;
    double   throughput_bps;
};

// =================================================================
// ============== 3. REUSABLE EXPERIMENT FUNCTION ==================
// =================================================================

// Map antenna strings -> ns-3 typeId names
static std::string
ResolveAntennaTypeId(const std::string& user)
{
    std::string s = user;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    if (s == "isotropic") return "ns3::IsotropicAntennaModel";
    if (s == "cosine")    return "ns3::CosineAntennaModel";
    if (s == "parabolic") return "ns3::ParabolicAntennaModel";
    return "ns3::IsotropicAntennaModel"; // Default
}

/**
 * @brief Executes a single LTE simulation with the given configuration.
 */
RunResult RunExperiment(const RunConfig& cfg)
{
    std::cout << "RUNNING: rate=" << cfg.dataRate
              << ", dist=" << cfg.distance << "m"
              << ", ant=" << cfg.antenna
              << ", orient=" << cfg.enbOrient << "/" << cfg.ueOrient
              << ", seed=" << cfg.seed
              << " ... " << std::flush;

    // --- Determinism ---
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(cfg.seed);

    // --- LTE/EPC Helpers and Attributes ---
    Ptr<LteHelper> lte = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epc = CreateObject<PointToPointEpcHelper>();
    lte->SetEpcHelper(epc);

    // --- Set parameters from the global constants section ---
    lte->SetSchedulerType(SCHEDULER);
    lte->SetAttribute("PathlossModel", StringValue(PATHLOSS_MODEL));
    lte->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(DL_EARFCN));
    lte->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(UL_EARFCN));
    lte->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(DL_BANDWIDTH_RBS));
    lte->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(UL_BANDWIDTH_RBS));
    
    Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));

    // --- Antenna configuration from the RunConfig ---
    const std::string antTypeId = ResolveAntennaTypeId(cfg.antenna);
    lte->SetEnbAntennaModelType(antTypeId);
    lte->SetUeAntennaModelType(antTypeId);
    if (antTypeId != "ns3::IsotropicAntennaModel")
    {
        lte->SetEnbAntennaModelAttribute("Orientation", DoubleValue(cfg.enbOrient));
        lte->SetUeAntennaModelAttribute("Orientation", DoubleValue(cfg.ueOrient));
    }

    // --- Nodes: eNB, UE, PGW, and Remote Server ---
    NodeContainer enbNodes; enbNodes.Create(1);
    NodeContainer ueNodes;  ueNodes.Create(1);
    Ptr<Node> pgw = epc->GetPgwNode();
    NodeContainer remoteHostCont; remoteHostCont.Create(1);
    
    InternetStackHelper internet;
    internet.Install(remoteHostCont);
    internet.Install(ueNodes);

    // --- Topology & Mobility ---
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);
    enbNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 0.0));
    ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(cfg.distance, 0.0, 0.0));

    // --- Install LTE Devices and Attach UE ---
    NetDeviceContainer enbDevs = lte->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueDevs  = lte->InstallUeDevice(ueNodes);
    Ipv4InterfaceContainer ueIfaces = epc->AssignUeIpv4Address(ueDevs);
    lte->Attach(ueDevs.Get(0), enbDevs.Get(0));

    // --- Core Network: PGW <-> Remote Server ---
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("5ms"));
    NetDeviceContainer internetDevs = p2p.Install(pgw, remoteHostCont.Get(0));

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIfs = ipv4h.Assign(internetDevs);

    // Static route on the remote server → UE subnet (EPC uses 7.0.0.0/8 by default)
    Ipv4StaticRoutingHelper srt;
    Ptr<Ipv4StaticRouting> rh = srt.GetStaticRouting(remoteHostCont.Get(0)->GetObject<Ipv4>());
    rh->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), internetIfs.GetAddress(0), 1);

    // --- Applications: UDP OnOff (server -> UE) ---
    const uint16_t port = 8000;
    const double appStart = 1.1;
    const double appStop = 10.0;
    const double simStop = 11.0;

    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(ueIfaces.GetAddress(0), port));
    onoff.SetAttribute("DataRate", StringValue(cfg.dataRate));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer client = onoff.Install(remoteHostCont.Get(0));
    client.Start(Seconds(appStart));
    client.Stop(Seconds(appStop));

    PacketSinkHelper sinkH("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkH.Install(ueNodes.Get(0));
    sinkApp.Start(Seconds(0.5));
    sinkApp.Stop(Seconds(simStop));
    
    // --- Tracing ---
    lte->EnablePdcpTraces();
    lte->EnableRlcTraces();
    p2p.EnablePcap("server_trace", internetDevs.Get(1), true);

    // --- Run Simulation ---
    Simulator::Stop(Seconds(simStop));
    Simulator::Run();

    // --- Collect Results ---
    uint64_t rxBytes = 0;
    if (sinkApp.GetN() > 0)
    {
        Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApp.Get(0));
        rxBytes = sink ? sink->GetTotalRx() : 0;
    }
    const double txWindow = appStop - appStart;
    const double throughput_bps = (rxBytes * 8.0) / txWindow;
    
    std::cout << "DONE. Throughput: " << (throughput_bps / 1e6) << " Mbps." << std::endl;

    // --- Cleanup ---
    Simulator::Destroy();

    return {rxBytes, throughput_bps};
}

// =================================================================
// ============== 4. MAIN BATCH DRIVER =============================
// =================================================================

int main(int argc, char* argv[])
{
    Time::SetResolution(Time::NS);

    // --- Define Parameter Sweeps ---
    const std::vector<std::string> antennaTypes = {"isotropic", "cosine", "parabolic"};
    const std::vector<std::string> dataRates    = {"1Mbps", "10Mbps", "20Mbps", "50Mbps"};
    const std::vector<double>      distances    = {50.0, 100.0, 250.0, 500.0, 1000.0};
    const uint32_t                 numRuns      = 2; // Number of runs per config for averaging

    // --- Prepare CSV Output File ---
    // Note: Creating a 'results' directory may be needed if it doesn't exist.
    std::ofstream ofs(CSV_OUTPUT_PATH, std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
    {
        std::cerr << "ERROR: Could not open CSV file: " << CSV_OUTPUT_PATH << std::endl;
        return 1;
    }
    // Write header
    ofs << "data_rate,distance_m,antenna,enb_orient_deg,ue_orient_deg,seed,rx_bytes,throughput_bps\n";

    std::cout << "==== Starting LTE Batch Experiment ====\n";
    std::cout << "Results will be saved to: " << CSV_OUTPUT_PATH << "\n\n";

    // --- Main Batch Loop ---
    for (const auto& antenna : antennaTypes)
    {
        for (const auto& rate : dataRates)
        {
            for (double dist : distances)
            {
                // Special case for directional antennas: test alignment and misalignment
                double enbOrient = 0.0;
                std::vector<double> ueOrients = {0.0}; // Default for isotropic
                if (antenna != "isotropic") {
                    std::vector<double> ueOrients = {}; // Empty vector to fill
                    std::generate_n(std::back_inserter(ueOrients), 12, [n = 0]() mutable {
                        double angle = n * 30.0;
                        ++n;
                        return angle;
                    });
                }

                for (double ueOrient : ueOrients)
                {
                    for (uint32_t i = 1; i <= numRuns; ++i)
                    {
                        RunConfig cfg = {antenna, dist, rate, enbOrient, ueOrient, i};
                        RunResult result = RunExperiment(cfg);

                        // Append result to CSV
                        ofs << cfg.dataRate << ","
                            << cfg.distance << ","
                            << cfg.antenna << ","
                            << cfg.enbOrient << ","
                            << cfg.ueOrient << ","
                            << cfg.seed << ","
                            << result.rxBytes << ","
                            << result.throughput_bps << "\n";
                    }
                }
            }
        }
    }

    ofs.close();
    std::cout << "\n==== Batch Experiment Complete ====\n";

    return 0;
}