#include "SimpleGroomExporter.h"

#include "GroomAsset.h"

THIRD_PARTY_INCLUDES_START
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
THIRD_PARTY_INCLUDES_END

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
//#include "Windows/HideWindowsPlatformTypes.h"
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

		std::vector<Imath::V3f> AllPositions;
		std::vector<int32_t> NumVertices;
		int32 GVertexIndex = 0;
		for (uint32 StrandIndex = 0; StrandIndex < GroupStrandData.GetNumCurves(); StrandIndex++)
		{
			int32 NumVertsInStrand = GroupStrandCurves.CurvesCount[StrandIndex];
			int32 Offset = GroupStrandCurves.CurvesOffset[StrandIndex];
			std::vector<Imath::V3f> PositionsForStrand;
			for (int32 LocalVertexIndex = 0; LocalVertexIndex < NumVertsInStrand; ++LocalVertexIndex)
			{
				PositionsForStrand.push_back(Imath::V3f(Positions[GVertexIndex].X, Positions[GVertexIndex].Y, Positions[GVertexIndex].Z));
				GVertexIndex++;
			}
			AllPositions.insert(AllPositions.end(), PositionsForStrand.begin(), PositionsForStrand.end());
			NumVertices.push_back(static_cast<int32_t>(PositionsForStrand.size()));
		}
		OCurvesSchema::Sample CurveSample
		(
			Abc::V3fArraySample(AllPositions.data(), AllPositions.size())
			, Abc::Int32ArraySample(NumVertices.data(), NumVertices.size())
			, kCubic // Curve type: cubic for this example
		);
		OCurves CurvesObj(TopLevelObj, std::string(TCHAR_TO_UTF8(*HairGroups.HairGroups[GroupIndex].Info.GroupName.ToString())));
		OCurvesSchema& CurvesSchema = CurvesObj.getSchema();
		CurvesSchema.set(CurveSample);
		
	}

	// for (int32 StrandIndex = 0; StrandIndex < TotalStrands; StrandIndex++)
	// {
	// 	FStrandID StrandID = FStrandID(StrandIndex);
	// 	const int GroupId = GroupIDAttr.IsValid() ? GroupIDAttr.Get(StrandID) : 0;
	//
	// 	if (!PositionsPerGroup.Contains(GroupId)) PositionsPerGroup.Add(GroupId, {});
	// 	if (!NumVerticesPerGroup.Contains(GroupId)) NumVerticesPerGroup.Add(GroupId, {});
	// 	if (!GlobalVertexIndexPerGroup.Contains(GroupId)) GlobalVertexIndexPerGroup.Add(GroupId, 0);
	//
	// 	const int32 NumVertsInStrand = NumVertsPerStrand.Get(StrandID);
	// 	std::vector<Imath::V3f> PositionsForStrand;
	// 	for (int32 LocalVertexIndex = 0; LocalVertexIndex < NumVertsInStrand; LocalVertexIndex++)
	// 	{
	// 		FVector3f Position = VertexPositions.Get(GlobalVertexIndexPerGroup.FindChecked(GroupId));
	// 		PositionsForStrand.push_back(Imath::V3f(Position.X, Position.Y, Position.Z));
	// 		GlobalVertexIndexPerGroup.FindChecked(GroupId)++;
	// 	}
	// 	PositionsPerGroup.FindChecked(GroupId).insert(PositionsPerGroup.FindChecked(GroupId).end(), PositionsForStrand.begin(), PositionsForStrand.end());
	// 	NumVerticesPerGroup.FindChecked(GroupId).push_back(static_cast<int32_t>(PositionsForStrand.size()));
	// }
	//
	// for (int32 GroupIndex = 0; GroupIndex < NumHairGroups; GroupIndex++)
	// {
	// 	OCurvesSchema::Sample CurveSample
	// 	(
	// 		Abc::V3fArraySample(PositionsPerGroup.FindChecked(GroupIndex).data(), PositionsPerGroup.FindChecked(GroupIndex).size())
	// 		, Abc::Int32ArraySample(NumVerticesPerGroup.FindChecked(GroupIndex).data(), NumVerticesPerGroup.FindChecked(GroupIndex).size())
	// 		, kCubic // i dont think this really matters for our purposes but also i dont really know what im doing
	// 	);
	// 	FString ObjName;
	// 	if (GroupNameAttr.IsValid())
	// 	{
	// 		ObjName = GroupNameAttr.Get(GroupIndex).ToString();
	// 	}
	// 	else
	// 	{
	// 		ObjName = NumHairGroups == 0 ? Groom->GetName() : Groom->GetName() + "_" + FString::FromInt(GroupIndex);
	// 	}
	// 	OCurves CurvesObj(TopLevelObj, std::string(TCHAR_TO_UTF8(*ObjName)));
	// 	OCurvesSchema& CurvesSchema = CurvesObj.getSchema();
	// 	CurvesSchema.set(CurveSample);
	// }

	////////////////

	// std::vector<Imath::V3f> AllPositions;
	// std::vector<int32_t> NumVertices;
	//
	// for (int32 StrandIndex = 0; StrandIndex < TotalStrands; StrandIndex++)
	// {
	// 	FStrandID StrandID = FStrandID(StrandIndex);
	// 	const int32 NumVertsInStrand = NumVertsPerStrand.Get(StrandID);
	// 	std::vector<Imath::V3f> PositionsForStrand;
	// 	for (int32 LocalVertexIndex = 0; LocalVertexIndex < NumVertsInStrand; LocalVertexIndex++)
	// 	{
	// 		FVector3f Position = VertexPositions.Get(GlobalVertexIndex);
	// 		PositionsForStrand.push_back(Imath::V3f(Position.X, Position.Y, Position.Z));
	// 		GlobalVertexIndex++;
	// 	}
	// 	AllPositions.insert(AllPositions.end(), PositionsForStrand.begin(), PositionsForStrand.end());
	// 	NumVertices.push_back(static_cast<int32_t>(PositionsForStrand.size()));
	// }

	// OCurvesSchema::Sample CurveSample
	// (
	// 	Abc::V3fArraySample(AllPositions.data(), AllPositions.size())
	// 	, Abc::Int32ArraySample(NumVertices.data(), NumVertices.size())
	// 	, kCubic // Curve type: cubic for this example
	// );
	// OCurves CurvesObj(TopLevelObj, std::string(TCHAR_TO_UTF8(*Groom->GetName())));
	// OCurvesSchema& CurvesSchema = CurvesObj.getSchema();
	// CurvesSchema.set(CurveSample);
	return true;
}
