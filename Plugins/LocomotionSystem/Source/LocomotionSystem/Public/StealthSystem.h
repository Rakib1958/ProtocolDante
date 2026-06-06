#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTree.h"
#include "StealthSystem.generated.h"

class UGoal_Base;

USTRUCT(BlueprintType)
struct FStruct_InfiltrationProfile
{
	GENERATED_BODY()

	// Player defaults to 0, Enemy Security to 1, Outlaws/Rivals to 2+
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infiltration | Identity")
	uint8 TeamNumber = 0;

	// Is the character actively using a disguise mask?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infiltration | Identity")
	bool bIsDisguised = false;

	// The active identity string of the worn outfit (e.g., "Guard_Sengoku", "Civilian_Servant")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infiltration | Identity")
	FName ActiveOutfitArchetype = NAME_None;
};

USTRUCT(BlueprintType)
struct FStruct_StealthActivity
{
	GENERATED_BODY()

	// The specific subclass of Goal that triggers this execution routine
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity Settings")
	TSubclassOf<UGoal_Base> SupportedGoalClass = nullptr;

	// The explicit Behavior Tree asset mapped to this scenario
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity Settings")
	UBehaviorTree* BehaviorTree = nullptr;

	// If true, a higher priority evaluation goal can cleanly break this execution loop
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity Settings")
	bool bIsInterruptible = true;
};

USTRUCT(BlueprintType)
struct FStruct_NPCDataAssetPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Save Data")
	bool bIsDead = false;

	/*UPROPERTY(BlueprintReadWrite, Category = "Save Data")
	bool bIsStripped = false;*/
	// Supporting Nemesis/Orc-style randomized visual generation tracking
	UPROPERTY(BlueprintReadWrite, Category = "Save Data")
	FString NPCIdentityName = TEXT("Unnamed Guard");

	// This acts as a perfect drop-in placeholder for Narrative Inventory IDs later!
	UPROPERTY(BlueprintReadWrite, Category = "Save Data")
	TArray<FString> EquippedVisualTags;

	UPROPERTY(BlueprintReadWrite, Category = "Save Data")
	FVector SavedLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Save Data")
	FRotator SavedRotation = FRotator::ZeroRotator;
};