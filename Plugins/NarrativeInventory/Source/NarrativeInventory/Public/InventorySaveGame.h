// Copyright Narrative Tools 2025. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NarrativeItem.h"
#include "InventorySaveGame.generated.h"

USTRUCT()
struct FNarrativeSavedItem
{
GENERATED_BODY()

public:

	FNarrativeSavedItem()
	{
		Quantity = 0;
		bActive = false;
		bFavourited = false;
	};

	FNarrativeSavedItem(class UNarrativeItem* Item) 
	{
		if (Item)
		{
			ItemClass = Item->GetClass();
			Quantity = Item->GetQuantity();
			bActive = Item->bActive;
			bFavourited = Item->bFavourite;
		}
	};

	UPROPERTY(SaveGame)
	TSubclassOf<class UNarrativeItem> ItemClass;

	UPROPERTY(SaveGame)
	int32 Quantity;

	UPROPERTY(SaveGame)
	bool bActive;

	UPROPERTY(SaveGame)
	bool bFavourited;

};

/**
 * Save file that holds the items in an inventory, and also restores their activated state 
 */
UCLASS()
class NARRATIVEINVENTORY_API UInventorySaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:

	UPROPERTY()
	TArray<FNarrativeSavedItem> SavedItems;

	UPROPERTY()
	int32 SavedCurrency;

};
