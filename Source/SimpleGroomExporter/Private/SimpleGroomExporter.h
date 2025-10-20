#pragma once
#include "CoreMinimal.h"
#include "Exporters/Exporter.h"
#include "SimpleGroomExporter.generated.h"

UCLASS(BlueprintType, Category= "Mask Export")
class SIMPLEGROOMEXPORTER_API USimpleGroomExportOptions : public UObject
{
	public:
		GENERATED_BODY()

		// Whether to use render data instead of raw data for export
		UPROPERTY(EditAnywhere, Category = "Export Options", meta = (DisplayName = "Use Render Data", ToolTip="Whether to use render data instead of raw data for export, groom must NOT be decimated"))
		bool bUseRenderData = false;
};

UCLASS()
class SIMPLEGROOMEXPORTER_API USimpleGroomExporter : public UExporter
{
	public:
		GENERATED_BODY()

		USimpleGroomExporter();
		virtual bool ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex = 0, uint32 PortFlags = 0) override;
		FString GetExportFilePath() const;
};
