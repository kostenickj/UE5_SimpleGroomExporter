// SimpleGroomExporter.cpp

#include "SimpleGroomExporter.h"

#include "HAL/Platform.h"
#include "GroomAsset.h"
#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include "Misc/FeedbackContext.h"
#include "Serialization/Archive.h"
#include "UObject/Object.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SUniformGridPanel.h"

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

#define LOCTEXT_NAMESPACE "SimpleGroomExporter"

class SSimpleGroomExporterExportDialog : public SWindow
{
	public:
		struct FResult
		{
			EAppReturnType::Type RetType;
			USimpleGroomExportOptions* Options = nullptr;

			FResult() :
			RetType(EAppReturnType::Type::Cancel)
			{
			}
		};


		SLATE_BEGIN_ARGS(SSimpleGroomExporterExportDialog)
			{
			}

		SLATE_END_ARGS()

		SSimpleGroomExporterExportDialog()
		: Config(nullptr)
		{
		}

		void Construct
		(
			const FArguments& InArgs
		)
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			Result = FResult();
			FDetailsViewArgs DetailsViewArgs;
			DetailsViewArgs.bUpdatesFromSelection = false;
			DetailsViewArgs.bLockable = false;
			DetailsViewArgs.bAllowSearch = false;
			DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
			DetailsViewArgs.bHideSelectionTip = true;
			DetailsViewArgs.NotifyHook = nullptr;
			DetailsViewArgs.bSearchInitialKeyFocus = false;
			DetailsViewArgs.ViewIdentifier = NAME_None;
			DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
			DetailsViewArgs.bShowOptions = false;
			DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
			SettingsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
			Config = NewObject<USimpleGroomExportOptions>();
			SettingsPanel->SetObject(Config);

			SWindow::Construct
			(
				SWindow::FArguments()
				.Title(LOCTEXT("SSimpleGroomExporterOptions", "Export Options"))
				.SupportsMinimize(false)
				.SupportsMaximize(false)
				.ClientSize(FVector2D(900, 500))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot() // Add user input block
					.Padding(2)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 10)
							[
								SettingsPanel->AsShared()
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SSeparator)
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					.Padding(5)
					[
						SNew(SUniformGridPanel)
						.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
						.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
						.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
						+ SUniformGridPanel::Slot(0, 0)
						.HAlign(HAlign_Left)
						[
							SNew(SButton)
							.HAlign(HAlign_Left)
							.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
							.Text(LOCTEXT("Cancel", "Cancel"))
							.OnClicked(this, &SSimpleGroomExporterExportDialog::OnButtonClick, EAppReturnType::Cancel)
							.IsEnabled(true)
						]
						+ SUniformGridPanel::Slot(2, 0)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
							.Text(LOCTEXT("ExportText", "Export"))
							.IsEnabled_Lambda
							(
								[this]()
								{
									return true;
								}
							)
							.OnClicked(this, &SSimpleGroomExporterExportDialog::OnButtonClick, EAppReturnType::Continue)
						]

					]
				]
			);
		}

		FResult ShowModal()
		{
			GEditor->EditorAddModalWindow(SharedThis(this));
			return this->Result;
		}
		
	private:
		USimpleGroomExportOptions* Config;
		TSharedPtr<IDetailsView> SettingsPanel;
		FResult Result;

		FReply OnButtonClick(EAppReturnType::Type ButtonID)
		{
			Result.RetType = ButtonID;
			Result.Options = Config;
			RequestDestroyWindow();
			return FReply::Handled();
		}
};


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

	// Show export options modal dialog
	TSharedRef<SSimpleGroomExporterExportDialog> OptionsModal = SNew(SSimpleGroomExporterExportDialog);
	SSimpleGroomExporterExportDialog::FResult ExportOptions = OptionsModal->ShowModal();
	if (ExportOptions.RetType == EAppReturnType::Cancel)
	{
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

		// Use the option selected by the user from the modal dialog
		const bool bUseRenderdataForExport = ExportOptions.Options->bUseRenderData;

		// TODO, consider using raw data hair desc data to export
		// see how GenerateHairCardGenGroomData access it....
		auto HairDesc = Groom->GetHairDescription();

		// Iterate over each hair group
		int32 GroupIdx = 0;
		for (const FHairDescriptionGroup& HairGroupInfo : HairGroups.HairGroups)
		{
			const FHairStrandsRawDatas& RawStrandData = HairGroupInfo.Strands;
			FHairStrandsDatas StrandData;
			FHairStrandsDatas GuidesData;
			if (bUseRenderdataForExport)
			{
				Groom->GetHairStrandsDatas(GroupIdx, StrandData, GuidesData);
			}

			const FHairStrandsCurves& StrandCurves = bUseRenderdataForExport ? StrandData.StrandsCurves : RawStrandData.StrandsCurves;
			const FHairStrandsPoints& StrandPoints = bUseRenderdataForExport ? StrandData.StrandsPoints : RawStrandData.StrandsPoints;
			const TArray<FVector3f>& Positions = StrandPoints.PointsPosition;

			// Prepare containers for Alembic data
			std::vector<Imath::V3f> AllPositions;
			std::vector<int32_t> VerticesPerStrand;

			AllPositions.reserve(Positions.Num());
			VerticesPerStrand.reserve(StrandCurves.CurvesCount.Num());


			const TArray<uint16>& CurvesCounts = StrandCurves.CurvesCount;
			const TArray<uint32>& CurvesOffsets = StrandCurves.CurvesOffset;
			const int32 NumStrands = CurvesCounts.Num();
			int32 GroupVertexIndex = 0;
			for (int32 StrandIndex = 0; StrandIndex < NumStrands; ++StrandIndex)
			{
				uint32 NumVertsInStrand = CurvesCounts[StrandIndex];
				if (bUseRenderdataForExport)
				{
					NumVertsInStrand = NumVertsInStrand - 1;
				}
				if (NumVertsInStrand <= 0)
				{
					UE_LOG(LogSimpleGroomExporter, Warning, TEXT("Strand %d has non-positive vertex count (%d). Skipping."), StrandIndex, NumVertsInStrand);
					continue;
				}

				// Collect positions for the current strand, curve offsets only seem to be set when using RENDER data, not hair strands raw datas
				uint32 StartVertexIndex = CurvesOffsets[StrandIndex];

				for (uint32 VertexIndexInStrand = 0; VertexIndexInStrand < NumVertsInStrand; ++VertexIndexInStrand)
				{
					uint32 CurrentVertexIndex = bUseRenderdataForExport ? StartVertexIndex + VertexIndexInStrand : GroupVertexIndex;
					if (CurrentVertexIndex >= static_cast<uint32>(Positions.Num()))
					{
						UE_LOG(LogSimpleGroomExporter, Error, TEXT("Vertex index out of bounds: %d >= %d"), CurrentVertexIndex, Positions.Num());
						return false;
					}
					const FVector3f& Pos = Positions[CurrentVertexIndex];
					AllPositions.emplace_back(Pos.X, Pos.Y, Pos.Z);
					GroupVertexIndex++;
				}

				VerticesPerStrand.emplace_back(NumVertsInStrand);
			}

			// Create Alembic curve sample
			OCurvesSchema::Sample CurveSample
			(
				V3fArraySample(AllPositions.data(), AllPositions.size())
				, Int32ArraySample(VerticesPerStrand.data(), VerticesPerStrand.size())
				, kCubic // Type of curve; can be adjusted as needed
			);

			// Define the name for the Alembic curves object
			std::string CurveName = std::string(TCHAR_TO_UTF8(*HairGroupInfo.Info.GroupName.ToString()));

			// Create the Alembic curves object and set its schema
			OCurves CurvesObj(TopLevelObj, CurveName);
			OCurvesSchema& CurvesSchema = CurvesObj.getSchema();
			CurvesSchema.set(CurveSample);

			UE_LOG(LogSimpleGroomExporter, Log, TEXT("Exported hair group: %s"), *HairGroupInfo.Info.GroupName.ToString());
			GroupIdx++;
		}

		UE_LOG(LogSimpleGroomExporter, Log, TEXT("Alembic export completed successfully: %s"), *OutputFileName);
		return true;
	}
	catch (const std::exception& Ex)
	{
		UE_LOG(LogSimpleGroomExporter, Error, TEXT("Alembic export failed: %s"), UTF8_TO_TCHAR(Ex.what()));
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

#undef LOCTEXT_NAMESPACE
