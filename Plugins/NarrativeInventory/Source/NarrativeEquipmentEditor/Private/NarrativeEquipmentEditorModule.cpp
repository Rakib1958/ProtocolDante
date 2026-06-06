// Copyright Narrative Tools 2025. 

#include "NarrativeEquipmentEditorModule.h"
#include "NarrativeEquipmentStyle.h"
#include "UObject/CoreRedirects.h"
#include "AssetTypeActions_EquippableItem.h"

DEFINE_LOG_CATEGORY(LogNarrativeEquipmentEditor);

#define LOCTEXT_NAMESPACE "FNarrativeEquipmentEditorModule"

uint32 FNarrativeEquipmentEditorModule::GameAssetCategory;

void FNarrativeEquipmentEditorModule::StartupModule()
{
	FNarrativeEquipmentStyle::Initialize();

	MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
	ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

	IAssetTools& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	GameAssetCategory = AssetToolsModule.FindAdvancedAssetCategory(FName("Narrative"));

	if (GameAssetCategory == EAssetTypeCategories::Misc)
	{
		GameAssetCategory = AssetToolsModule.RegisterAdvancedAssetCategory(FName(TEXT("Narrative")), LOCTEXT("NarrativeCategory", "Narrative"));
	}

	TSharedPtr<FAssetTypeActions_EquippableItem> NarrativeEquippableTypeAction = MakeShareable(new FAssetTypeActions_EquippableItem(GameAssetCategory));
	NarrativeEquippableTypeActions = NarrativeEquippableTypeAction;
	AssetToolsModule.RegisterAssetTypeActions(NarrativeEquippableTypeAction.ToSharedRef());

	TSharedPtr<FAssetTypeActions_ClothingItem> NarrativeClothingTypeAction = MakeShareable(new FAssetTypeActions_ClothingItem(GameAssetCategory));
	NarrativeClothingTypeActions = NarrativeClothingTypeAction;
	AssetToolsModule.RegisterAssetTypeActions(NarrativeClothingTypeAction.ToSharedRef());
}


void FNarrativeEquipmentEditorModule::ShutdownModule()
{
	FNarrativeEquipmentStyle::Shutdown();

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (NarrativeEquippableTypeActions.IsValid())
		{
			AssetToolsModule.UnregisterAssetTypeActions(NarrativeEquippableTypeActions.ToSharedRef());
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNarrativeEquipmentEditorModule, NarrativeEquipmentEditor)
