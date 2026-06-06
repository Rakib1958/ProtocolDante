// Copyright Narrative Tools 2025. 

#include "InventoryFunctionLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "InventoryComponent.h"
#include "NarrativeItem.h"

class UNarrativeInventoryComponent* UInventoryFunctionLibrary::GetInventoryComponentFromTarget(AActor* Target)
{
	if (!Target)
	{
		return nullptr;
	}

	if (UNarrativeInventoryComponent* InventoryComp = Target->FindComponentByClass<UNarrativeInventoryComponent>())
	{
		return InventoryComp;
	}

	//Try player state, then pawn, then controller
	if (const APawn* OwningPawn = Cast<APawn>(Target))
	{
		if (const APlayerState* PlayerState = OwningPawn->GetPlayerState<APlayerState>())
		{
			if (UNarrativeInventoryComponent* InventoryComp = PlayerState->FindComponentByClass<UNarrativeInventoryComponent>())
			{
				return InventoryComp;
			}
		}

		if (const AController* OwningController = OwningPawn->GetController())
		{
			if (UNarrativeInventoryComponent* InventoryComp = OwningController->FindComponentByClass<UNarrativeInventoryComponent>())
			{
				return InventoryComp;
			}
		}
	}
	else if (const APlayerController* OwningController = Cast<APlayerController>(Target))
	{
		if (OwningController->GetPawn())
		{
			if (const APlayerState* PlayerState = OwningController->GetPlayerState<APlayerState>())
			{
				if (UNarrativeInventoryComponent* InventoryComp = PlayerState->FindComponentByClass<UNarrativeInventoryComponent>())
				{
					return InventoryComp;
				}
			}

			return OwningController->GetPawn()->FindComponentByClass<UNarrativeInventoryComponent>();
		}
	}

	return nullptr;
}


TArray<class UNarrativeItem*> UInventoryFunctionLibrary::SortItemArrayAlphabetical(TArray<class UNarrativeItem*> InItems, const bool bReverse)
{

	TArray<class UNarrativeItem*> RetItems = InItems;

	//Sort the replies by their Y position in the graph
	if (bReverse)
	{
		RetItems.Sort([](const UNarrativeItem& ItemA, const UNarrativeItem& ItemB) {
			return ItemA.DisplayName.ToString() < ItemB.DisplayName.ToString();
			});
	}
	else
	{
		RetItems.Sort([](const UNarrativeItem& ItemA, const UNarrativeItem& ItemB) {
			return ItemA.DisplayName.ToString() > ItemB.DisplayName.ToString();
			});
	}

	return RetItems;
}

TArray<class UNarrativeItem*> UInventoryFunctionLibrary::SortItemArrayWeight(TArray<class UNarrativeItem*> InItems, const bool bReverse)
{
	TArray<class UNarrativeItem*> RetItems = InItems;

	//Sort the replies by their Y position in the graph
	if (bReverse)
	{
		RetItems.Sort([](const UNarrativeItem& ItemA, const UNarrativeItem& ItemB) {
			return ItemA.GetStackWeight() > ItemB.GetStackWeight();
			});
	}
	else
	{
		RetItems.Sort([](const UNarrativeItem& ItemA, const UNarrativeItem& ItemB) {
			return ItemA.GetStackWeight() < ItemB.GetStackWeight();
			});
	}

	
	return RetItems;
}
