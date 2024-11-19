// SimpleGroomExporter.cpp

#include "SimpleGroomExporter.h"

#include "HAL/Platform.h"
#include "GroomAsset.h"
#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include "Misc/FeedbackContext.h"
#include "Serialization/Archive.h"
#include "UObject/Object.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

THIRD_PARTY_INCLUDES_START
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
THIRD_PARTY_INCLUDES_END

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

using namespace Alembic::Abc;
using namespace Alembic::AbcGeom;

// Define a logging category for the exporter
DEFINE_LOG_CATEGORY_STATIC(LogSimpleGroomExporter, Log, All);

USimpleGroomExporter::USimpleGroomExporter()
{
    SupportedClass = UGroomAsset::StaticClass();
    bText = false;
    FormatExtension.Add(TEXT("ABC"));
    FormatDescription.Add(TEXT("Alembic File"));
}

bool USimpleGroomExporter::ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags)
{
    // Validate input object
    if (!Object)
    {
        UE_LOG(LogSimpleGroomExporter, Error, TEXT("Export failed: Object is null."));
        return false;
    }

    // Ensure the object is of type UGroomAsset
    UGroomAsset* Groom = Cast<UGroomAsset>(Object);
    if (!Groom)
    {
        UE_LOG(LogSimpleGroomExporter, Error, TEXT("Export failed: Object is not a UGroomAsset."));
        return false;
    }

    // Retrieve the output filename
    FString OutputFileName = GetExportFilePath();
    if (OutputFileName.IsEmpty())
    {
        UE_LOG(LogSimpleGroomExporter, Error, TEXT("Export failed: Output file path is empty."));
        return false;
    }

    // Initialize Alembic archive
    try
    {
        // Convert FString to std::string using UTF-8 encoding
        std::string OutputFileNameStd = TCHAR_TO_UTF8(*OutputFileName);
        OArchive Archive(Alembic::AbcCoreOgawa::WriteArchive(), OutputFileNameStd);
        OObject TopLevelObj = Archive.getTop();

        // Retrieve hair groups from the groom asset
        const FHairDescriptionGroups& HairGroups = Groom->GetHairDescriptionGroups();
        if (HairGroups.HairGroups.Num() == 0)
        {
            UE_LOG(LogSimpleGroomExporter, Warning, TEXT("No hair groups found in the groom asset."));
            return false;
        }

        // Iterate over each hair group
        for (const FHairDescriptionGroup& HairGroupInfo : HairGroups.HairGroups)
        {
            const FHairStrandsDatas& StrandData = HairGroupInfo.Strands;
            const FHairStrandsCurves& StrandCurves = StrandData.StrandsCurves;
            const FHairStrandsPoints& StrandPoints = StrandData.StrandsPoints;
            const TArray<FVector3f>& Positions = StrandPoints.PointsPosition;

            // Prepare containers for Alembic data
            std::vector<Imath::V3f> AllPositions;
            std::vector<int32_t> VerticesPerStrand;

            // Reserve space to optimize memory allocations
            AllPositions.reserve(Positions.Num());
            VerticesPerStrand.reserve(StrandCurves.CurvesCount.Num());

            int32 GroupVertexIndex = 0;
            const int32 NumStrands = StrandCurves.CurvesCount.Num();

            for (int32 StrandIndex = 0; StrandIndex < NumStrands; ++StrandIndex)
            {
                int32 NumVertsInStrand = StrandCurves.CurvesCount[StrandIndex];
                if (NumVertsInStrand <= 0)
                {
                    UE_LOG(LogSimpleGroomExporter, Warning, TEXT("Strand %d has non-positive vertex count (%d). Skipping."), StrandIndex, NumVertsInStrand);
                    continue;
                }

                // Collect positions for the current strand
                for (int32 VertexIndex = 0; VertexIndex < NumVertsInStrand; ++VertexIndex)
                {
                    if (GroupVertexIndex >= Positions.Num())
                    {
                        UE_LOG(LogSimpleGroomExporter, Error, TEXT("Vertex index out of bounds: %d >= %d"), GroupVertexIndex, Positions.Num());
                        return false;
                    }
                    const FVector3f& Pos = Positions[GroupVertexIndex++];
                    AllPositions.emplace_back(Pos.X, Pos.Y, Pos.Z);
                }

                VerticesPerStrand.emplace_back(NumVertsInStrand);
            }

            // Create Alembic curve sample
            OCurvesSchema::Sample CurveSample(
                V3fArraySample(AllPositions.data(), AllPositions.size()),
                Int32ArraySample(VerticesPerStrand.data(), VerticesPerStrand.size()),
                kCubic // Type of curve; can be adjusted as needed
            );

            // Define the name for the Alembic curves object
            std::string CurveName = std::string(TCHAR_TO_UTF8(*HairGroupInfo.Info.GroupName.ToString()));

            // Create the Alembic curves object and set its schema
            OCurves CurvesObj(TopLevelObj, CurveName);
            OCurvesSchema& CurvesSchema = CurvesObj.getSchema();
            CurvesSchema.set(CurveSample);

            UE_LOG(LogSimpleGroomExporter, Log, TEXT("Exported hair group: %s"), *HairGroupInfo.Info.GroupName.ToString());
        }

        UE_LOG(LogSimpleGroomExporter, Log, TEXT("Alembic export completed successfully: %s"), *OutputFileName);
        return true;
    }
    catch (const std::exception& Ex)
    {
        UE_LOG(LogSimpleGroomExporter, Error, TEXT("Alembic export failed: %hs"), UTF8_TO_TCHAR(Ex.what()));
        return false;
    }
    catch (...)
    {
        UE_LOG(LogSimpleGroomExporter, Error, TEXT("Alembic export failed due to an unknown error."));
        return false;
    }
}

FString USimpleGroomExporter::GetExportFilePath() const
{
    return UExporter::CurrentFilename;
}
