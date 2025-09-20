#include <jde/framework/settings.h>
#include "../src/globals.h"
#include "../src/UAServer.h"

namespace Jde::Opc::Server::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct UALoadTests : ::testing::Test{
	protected:
		Ω SetUpTestCase()ι->void{}
		Ω TearDownTestCase()ι->void{}
		α SetUp()ι->void{
			Server::Initialize( ServerId(), GetSchemaPtr() );
		}
		Ω Path()ι->fs::path{ return *Settings::FindPath( "/testing/UANodeSets"); }
	};

	TEST_F( UALoadTests, CommercialKitchenEquipment ){
		GetUAServer().Load( Path()/"CommercialKitchenEquipment/Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, Server_loadADINodeset){
		GetUAServer().Load( Path()/"ADI/Opc.Ua.Adi.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadAMBNodeset){
		GetUAServer().Load( Path()/"AMB/Opc.Ua.AMB.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadAMLBaseTypesNodeset){
		GetUAServer().Load( Path()/"AML/Opc.Ua.AMLBaseTypes.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadAutoIDNodeset){
		GetUAServer().Load( Path()/"AutoID/Opc.Ua.AutoID.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadBACnetNodeset){
		GetUAServer().Load( Path()/"BACnet/Opc.Ua.BACnet.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadCASNodeset){
		GetUAServer().Load( Path()/"CAS/Opc.Ua.CAS.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadCommercialKitchenEquipmentNodeset){
		GetUAServer().Load( Path()/"CommercialKitchenEquipment/Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadCSPPlusForMachineNodeset){
		GetUAServer().Load( Path()/"CSPPlusForMachine/Opc.Ua.CSPPlusForMachine.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadDEXPINodeset){
		GetUAServer().Load( Path()/"DEXPI/Opc.Ua.DEXPI.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadDINodeset){
		GetUAServer().Load( Path()/"DI/Opc.Ua.Di.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadDotNetNodeset){
		GetUAServer().Load( Path()/"DotNet/Opc.Ua.NodeSet.xml" );
	}

	TEST_F( UALoadTests, LoadFDI5Nodeset){
		GetUAServer().Load( Path()/"FDI/Opc.Ua.Fdi5.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadFDI7Nodeset){
		GetUAServer().Load( Path()/"FDI/Opc.Ua.Fdi7.NodeSet2.xml" );
	}


	TEST_F( UALoadTests, LoadFDTNodeset){
		GetUAServer().Load( Path()/"FDT/Opc.Ua.FDT.NodeSet.xml" );
	}

	TEST_F( UALoadTests, LoadGDSNodeset){
		GetUAServer().Load( Path()/"GDS/Opc.Ua.Gds.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadServer_loadGlassNodeset){
		GetUAServer().Load( Path()/"Glass/Flat/Opc.Ua.Glass.NodeSet2.xml" );
	}


	TEST_F( UALoadTests, LoadI4AASNodeset){
		GetUAServer().Load( Path()/"I4AAS/Opc.Ua.I4AAS.NodeSet2.xml" );
	}


	TEST_F( UALoadTests, LoadIANodeset){
		GetUAServer().Load( Path()/"IA/Opc.Ua.IA.NodeSet2.xml" );
	}


	TEST_F( UALoadTests, LoadIAExamplesNodeset){
		GetUAServer().Load( Path()/"IA/Opc.Ua.IA.NodeSet2.examples.xml" );
	}


	TEST_F( UALoadTests, LoadIOLinkIODDNodeset){
		GetUAServer().Load( Path()/"IOLink/Opc.Ua.IOLinkIODD.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadIOLinkNodeset){
		GetUAServer().Load( Path()/"IOLink/Opc.Ua.IOLink.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadISA95Nodeset){
		GetUAServer().Load( Path()/"ISA-95/Opc.ISA95.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadMachineryNodeset){
		GetUAServer().Load( Path()/"Machinery/Opc.Ua.Machinery.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadMachineryExamplesNodeset){
		GetUAServer().Load( Path()/"Machinery/Opc.Ua.Machinery.Examples.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadMachineToolNodeset){
		GetUAServer().Load( Path()/"MachineTool/Opc.Ua.MachineTool.NodeSet2.xml" );
	}


	// TEST_F( UALoadTests, LoadMDISNodeset){
	//
	//  GetUAServer().Load( Path()/ "MDIS/Opc.MDIS.NodeSet2.xml" );
	//
	// }
	//

	TEST_F( UALoadTests, LoadMiningDevelopmentSupportGeneralNodeset){
		GetUAServer().Load( Path()/"Mining/DevelopmentSupport/General/1.0.0/Opc.Ua.Mining.DevelopmentSupport.General.NodeSet2.xml" );
	}


	TEST_F( UALoadTests, LoadMiningExtractionGeneralNodeset){
		GetUAServer().Load( Path()/"Mining/Extraction/General/1.0.0/Opc.Ua.Mining.Extraction.General.NodeSet2.xml" );
	}


	TEST_F( UALoadTests, LoadMiningMineralProcessingGeneralNodeset){
		GetUAServer().Load( Path()/"Mining/MineralProcessing/General/1.0.0/Opc.Ua.Mining.MineralProcessing.General.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadMiningMonitoringSupervisionServicesGeneralNodeset){
		GetUAServer().Load( Path()/"Mining/MonitoringSupervisionServices/General/1.0.0/Opc.Ua.Mining.MonitoringSupervisionServices.General.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadMTConnectNodeset){
		GetUAServer().Load( Path()/"MTConnect/Opc.Ua.MTConnect.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadOPENSCSNodeset){

		GetUAServer().Load( Path()/"OpenSCS/Opc.Ua.OPENSCS.NodeSet2.xml" );

	}

	TEST_F( UALoadTests, LoadPackMLNodeset){

		GetUAServer().Load( Path()/"PackML/Opc.Ua.PackML.NodeSet2.xml" );

	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionCalenderNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/Calender/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calender.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionCalibratorNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/Calibrator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calibrator.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionCorrugatorNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/Corrugator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Corrugator.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionCutterNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/Cutter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Cutter.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionDieNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/Die/1.00/Opc.Ua.PlasticsRubber.Extrusion.Die.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionExtruderNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/Extruder/1.00/Opc.Ua.PlasticsRubber.Extrusion.Extruder.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionExtrusionLineNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/ExtrusionLine/1.00/Opc.Ua.PlasticsRubber.Extrusion.ExtrusionLine.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionFilterNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/Filter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Filter.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionGeneralTypes_v1_0_1_Nodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionHaulOffNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionMeltPumpNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/MeltPump/1.00/Opc.Ua.PlasticsRubber.Extrusion.MeltPump.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionPelletizerNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion/Pelletizer/1.00/Opc.Ua.PlasticsRubber.Extrusion.Pelletizer.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2CalenderNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/Calender/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calender.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2CalibratorNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/Calibrator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calibrator.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2CorrugatorNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/Corrugator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Corrugator.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2CutterNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/Cutter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Cutter.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2DieNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/Die/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Die.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2ExtruderNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/Extruder/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Extruder.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2ExtrusionLineNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/ExtrusionLine/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.ExtrusionLine.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2FilterNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/Filter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Filter.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2GeneralTypesNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2HaulOffNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2MeltPumpNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/MeltPump/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.MeltPump.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberExtrusionv2PelletizerNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/Extrusion_v2/Pelletizer/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Pelletizer.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberGeneralTypes_v1_0_2_Nodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberGeneralTypes_v1_0_3_Nodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberHotRunnerNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/HotRunner/1.00/Opc.Ua.PlasticsRubber.HotRunner.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberIMM2MESNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/IMM2MES/1.01/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberLDSNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/LDS/1.00/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPlasticsRubberTCDNodeset){
		GetUAServer().Load( Path()/"PlasticsRubber/TCD/1.01/Opc.Ua.PlasticsRubber.TCD.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPLCopenNodeset){
		GetUAServer().Load( Path()/"PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml" );
	}


	TEST_F( UALoadTests, LoadPnEmNodeset){
		GetUAServer().Load( Path()/"PNEM/Opc.Ua.PnEm.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadPnRioNodeset){
		GetUAServer().Load( Path()/"PNRIO/Opc.Ua.PnRio.Nodeset2.xml" );
	}

	TEST_F( UALoadTests, LoadPROFINETNodeset){
		GetUAServer().Load( Path()/"PROFINET/Opc.Ua.Pn.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadRoboticsNodeset){
		GetUAServer().Load( Path()/"Robotics/Opc.Ua.Robotics.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadSafetyNodeset){
		GetUAServer().Load( Path()/"Safety/Opc.Ua.Safety.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadSercosNodeset){
		GetUAServer().Load( Path()/"Sercos/Sercos.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadWeihenstephanNodeset){
		GetUAServer().Load( Path()/"Weihenstephan/Opc.Ua.Weihenstephan.NodeSet2.xml" );
	}

	TEST_F( UALoadTests, LoadWoodworkingEumaboisNodeset){
		GetUAServer().Load( Path()/"Woodworking/Opc.Ua.Eumabois.Nodeset2.xml" );
	}

	TEST_F( UALoadTests, LoadWoodworkingNodeset){
		GetUAServer().Load( Path()/"Woodworking/Opc.Ua.Woodworking.NodeSet2.xml" );
	}
}
