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

	FHairDescription HairDescription = Groom->GetHairDescription();
	const int32 TotalStrands = HairDescription.GetNumStrands();
	const TMeshAttributesConstRef<FStrandID, TMeshAttributesRefType<int>::ConstRefType> NumVertsPerStrand = HairDescription.StrandAttributes().GetAttributesRef<int>(HairAttribute::Strand::VertexCount);
	TMeshAttributesConstRef<FVertexID, TMeshAttributesRefType<FVector3f>::ConstRefType> VertexPositions = HairDescription.VertexAttributes().GetAttributesRef<FVector3f>(HairAttribute::Vertex::Position);

	// create alembic archive
	OArchive Archive(Alembic::AbcCoreOgawa::WriteArchive(), std::string(TCHAR_TO_UTF8(*OutputfileName)));
	OObject TopLevelObj = Archive.getTop();

	int32 GlobalVertexIndex = 0;
	for (int32 StrandIndex = 0; StrandIndex < TotalStrands; StrandIndex++)
	{
		FStrandID StrandID = FStrandID(StrandIndex);

		const int32 NumVertsInStrand = NumVertsPerStrand.Get(StrandID);
		std::vector<Imath::V3f> PositionsForStrand;
		for (int32 LocalVertexIndex = 0; LocalVertexIndex < NumVertsInStrand; LocalVertexIndex++)
		{
			FVector3f Position = VertexPositions.Get(GlobalVertexIndex);
			PositionsForStrand.push_back(Imath::V3f(Position.X, Position.Y, Position.Z));
			GlobalVertexIndex++;
		}
		std::vector<int32_t> NumVertices = {static_cast<int32_t>(PositionsForStrand.size())};

		OCurvesSchema::Sample CurveSample
		(
			Abc::V3fArraySample(PositionsForStrand.data(), PositionsForStrand.size())
			, Abc::Int32ArraySample(NumVertices.data(), NumVertices.size())
			, kCubic // Curve type: cubic for this example TODO kLinear ?
		);
		
		std::string CurveName = "strand" + std::to_string(StrandIndex + 1);
		OCurves CurvesObj(TopLevelObj, CurveName);
		OCurvesSchema& CurvesSchema = CurvesObj.getSchema();
		CurvesSchema.set(CurveSample);
	}
	
	return true;
}
