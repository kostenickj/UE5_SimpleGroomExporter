#include "SimpleGroomExporter.h"

#include "HAL/Platform.h"
#include "GroomAsset.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

THIRD_PARTY_INCLUDES_START
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
THIRD_PARTY_INCLUDES_END

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

using namespace Alembic::Abc;
using namespace Alembic::AbcGeom;

USimpleGroomExporter::USimpleGroomExporter()
{
	SupportedClass = UGroomAsset::StaticClass();
	bText = false;
	FormatExtension.Add(TEXT("ABC"));
	FormatDescription.Add(TEXT("ABC File"));
}

bool USimpleGroomExporter::ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags)
{
	const FString OutputfileName = UExporter::CurrentFilename;
	UGroomAsset* Groom = CastChecked<UGroomAsset>(Object);

	// create alembic archive
	OArchive Archive(Alembic::AbcCoreOgawa::WriteArchive(), std::string(TCHAR_TO_UTF8(*OutputfileName)));
	OObject TopLevelObj = Archive.getTop();

	const FHairDescriptionGroups& HairGroups = Groom->GetHairDescriptionGroups();

	for (int32 GroupIndex = 0; GroupIndex < HairGroups.HairGroups.Num(); GroupIndex++)
	{
		const FHairStrandsDatas& GroupStrandData = HairGroups.HairGroups[GroupIndex].Strands;
		const FHairStrandsCurves& GroupStrandCurves = GroupStrandData.StrandsCurves;
		const FHairStrandsPoints& GroupStrandPoints = GroupStrandData.StrandsPoints;
		const TArray<FVector3f>& Positions = GroupStrandPoints.PointsPosition;

		std::vector<Imath::V3f> AllPositionsForGroup;
		std::vector<int32_t> NumVerticesPerStrandsForGroup;
		
		int32 GroupVertexIndex = 0;
		for (uint32 StrandIndex = 0; StrandIndex < GroupStrandData.GetNumCurves(); StrandIndex++)
		{
			int32 NumVertsInStrand = GroupStrandCurves.CurvesCount[StrandIndex];
			std::vector<Imath::V3f> PositionsForStrand;
			for (int32 LocalVertexIndex = 0; LocalVertexIndex < NumVertsInStrand; ++LocalVertexIndex)
			{
				PositionsForStrand.push_back(Imath::V3f(Positions[GroupVertexIndex].X, Positions[GroupVertexIndex].Y, Positions[GroupVertexIndex].Z));
				GroupVertexIndex++;
			}
			AllPositionsForGroup.insert(AllPositionsForGroup.end(), PositionsForStrand.begin(), PositionsForStrand.end());
			NumVerticesPerStrandsForGroup.push_back(static_cast<int32_t>(PositionsForStrand.size()));
		}
		OCurvesSchema::Sample CurveSample
		(
			Abc::V3fArraySample(AllPositionsForGroup.data(), AllPositionsForGroup.size())
			, Abc::Int32ArraySample(NumVerticesPerStrandsForGroup.data(), NumVerticesPerStrandsForGroup.size())
			, kCubic  // i dont think this really matters for our purposes but also i dont really know what im doing
		);
		OCurves CurvesObj(TopLevelObj, std::string(TCHAR_TO_UTF8(*HairGroups.HairGroups[GroupIndex].Info.GroupName.ToString())));
		OCurvesSchema& CurvesSchema = CurvesObj.getSchema();
		CurvesSchema.set(CurveSample);
		
	}
	
	return true;
}
