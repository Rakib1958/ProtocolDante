#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "NPCClothingDefinition.h"
#include "AC_NPC_Clothing.generated.h"

// Forward declarations to ensure fast compilation
class USkeletalMesh;
class UAnimBlueprint;

USTRUCT(BlueprintType)
struct FBodyPartDefinition
{
    GENERATED_BODY()

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
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
    TMap<EBodyPart, FBodyPartDefinition> MeshParts;

    UPROPERTY(EditDefaultsOnly, Category = "Skeleton")
    ESkeletonType SkeletonType = ESkeletonType::UE5Mannequin;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TSoftObjectPtr<UAnimBlueprint> RetargetABP_UE4;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TSoftObjectPtr<UAnimBlueprint> RetargetABP_UE5;

    UPROPERTY(EditDefaultsOnly, Category = "Loot")
    TSubclassOf<AActor> LootableItemClass;

    UPROPERTY(EditDefaultsOnly, Category = "Loot")
    FName LootableItemID;
};

UCLASS(ClassGroup = "NPC", meta = (BlueprintSpawnableComponent))
class CHARACTERDEMO_API UAC_NPC_Clothing : public UActorComponent
{
    GENERATED_BODY()

public:
    UAC_NPC_Clothing();

    UPROPERTY(EditDefaultsOnly, Category = "Clothing")
    TObjectPtr<UNPCClothingDefinition> ClothingDefinition;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bClothingLoaded = false;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bUniformLooted = false;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClothingLoaded);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnClothingLoaded OnClothingLoaded;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUniformLooted);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnUniformLooted OnUniformLooted;

    UFUNCTION(BlueprintCallable, Category = "Clothing")
    void InitializeClothing(UNPCClothingDefinition* Definition, bool bWasLooted);

    UFUNCTION(BlueprintCallable, Category = "Clothing")
    void LootUniform();

    void SetMeshLODBias(int32 LODBias);
    void SetMeshVisibility(bool bVisible);
    void SetFollowerTickInterval(float Interval);

private:
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> BodyMesh;

    UPROPERTY()
    TArray<TObjectPtr<USkeletalMeshComponent>> FollowerMeshes;

    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> UniformMesh;

    TSharedPtr<FStreamableHandle> AsyncLoadHandle;

    void ResolveMeshComponents();
    USkeletalMeshComponent* FindMeshByTag(FName Tag) const;
    void StartAsyncLoad();
    void OnAllMeshesLoaded();

    // FIX: Explicitly synchronized parameters to receive raw asset references cleanly
    void ApplyMeshToComponent(USkeletalMeshComponent* Comp,
        USkeletalMesh* Mesh,
        UAnimBlueprint* RetargetABP,
        const TMap<FName, float>& Morphs);

    void SetAllMeshesVisible(bool bVisible);
};