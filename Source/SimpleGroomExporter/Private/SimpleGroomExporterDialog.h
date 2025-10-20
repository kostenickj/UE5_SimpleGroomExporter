#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Widgets/SWindow.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "SimpleGroomExporterDialog.generated.h"

#define LOCTEXT_NAMESPACE "SimpleGroomExporter"

UCLASS(BlueprintType, Category= "Groom Export")
class SIMPLEGROOMEXPORTER_API USimpleGroomExportOptions : public UObject
{
	public:
		GENERATED_BODY()

		// Whether to use render data instead of raw data for export
		UPROPERTY(EditAnywhere, Category = "Export Options",  meta = (DisplayName = "Use Render Data", ToolTip="Whether to export groom render data instead of raw data for export, groom must NOT be decimated. This can sometime fix issues with the export."))
		bool bUseRenderData = false;
};

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

		SSimpleGroomExporterExportDialog();

		void Construct(const FArguments& InArgs);

		FResult ShowModal();

	private:
		USimpleGroomExportOptions* Config;
		TSharedPtr<IDetailsView> SettingsPanel;
		FResult Result;

		FReply OnButtonClick(EAppReturnType::Type ButtonID);
};

#undef LOCTEXT_NAMESPACE
