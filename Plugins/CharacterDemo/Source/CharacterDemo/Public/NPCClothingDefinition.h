#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NPCClothingDefinition.generated.h"

// Forward declarations to ensure lightning-fast compiler pass times
class USkeletalMesh;
class UAnimBlueprint;

UENUM(BlueprintType)
enum class ESkeletonType : uint8
{
    UE4Mannequin,
    UE5Mannequin
};

UENUM(BlueprintType)
enum class EBodyPart : uint8
{
    Body,
    Head,
    Hair,
    Uniform
};

USTRUCT(BlueprintType)
struct FBodyPartDefinition
{
    GENERATED_BODY()

    // FIX: Targets the true raw asset class, not a dynamic live scene component
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
    TSoftObjectPtr<USkeletalMesh> MeshAsset;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
    TMap<FName, float> MorphTargets;
};

UCLASS(BlueprintType)
class CHARACTERDEMO_API UNPCClothingDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // FIX: Combined individual variables into the structured Map layout expected by your component loops
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
    TMap<EBodyPart, FBodyPartDefinition> MeshParts;

    UPROPERTY(EditDefaultsOnly, Category = "Skeleton")
    ESkeletonType SkeletonType = ESkeletonType::UE5Mannequin;

    // FIX: Changed from UAnimInstance to UAnimBlueprint to match disk asset storage types
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TSoftObjectPtr<UAnimBlueprint> RetargetABP_UE4;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TSoftObjectPtr<UAnimBlueprint> RetargetABP_UE5;

    // FIX: Matched the TSubclassOf type layout required by your inventory item spans
    UPROPERTY(EditDefaultsOnly, Category = "Loot")
    TSubclassOf<AActor> LootableItemClass;

    UPROPERTY(EditDefaultsOnly, Category = "Loot")
    FName LootableItemID;
};