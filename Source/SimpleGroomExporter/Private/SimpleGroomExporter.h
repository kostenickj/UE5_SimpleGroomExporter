#pragma once
#include "CoreMinimal.h"
#include "Exporters/Exporter.h"
#include "SimpleGroomExporter.generated.h"

UCLASS()
class SIMPLEGROOMEXPORTER_API USimpleGroomExporter : public UExporter
{
	public:
		GENERATED_BODY()

		USimpleGroomExporter();
		virtual bool ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex = 0, uint32 PortFlags = 0) override;
		FString GetExportFilePath() const;
};
