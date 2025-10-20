#include "SimpleGroomExporterDialog.h"
#include "Editor.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "SimpleGroomExporter"

SSimpleGroomExporterExportDialog::SSimpleGroomExporterExportDialog()
: Config(nullptr)
{
}

void SSimpleGroomExporterExportDialog::Construct(const FArguments& InArgs)
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

SSimpleGroomExporterExportDialog::FResult SSimpleGroomExporterExportDialog::ShowModal()
{
	GEditor->EditorAddModalWindow(SharedThis(this));
	return this->Result;
}

FReply SSimpleGroomExporterExportDialog::OnButtonClick(EAppReturnType::Type ButtonID)
{
	Result.RetType = ButtonID;
	Result.Options = Config;
	RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
