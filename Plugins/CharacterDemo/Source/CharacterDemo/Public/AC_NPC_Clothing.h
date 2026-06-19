#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "NPCClothingDefinition.h" // FIX: Include the asset header to get structural visibility
#include "AC_NPC_Clothing.generated.h"

class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimBlueprint;

UCLASS(ClassGroup = "NPC", meta = (BlueprintSpawnableComponent))
class CHARACTERDEMO_API UAC_NPC_Clothing : public UActorComponent
{
    GENERATED_BODY()

public:
    UAC_NPC_Clothing();

    // ── Configuration ────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, Category = "Clothing")
    TObjectPtr<UNPCClothingDefinition> ClothingDefinition;

    // ── State ────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bClothingLoaded = false;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bUniformLooted = false;

    // ── Delegates ────────────────────────────────────────────────────────

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClothingLoaded);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnClothingLoaded OnClothingLoaded;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUniformLooted);
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnUniformLooted OnUniformLooted;

    // ── Public API ───────────────────────────────────────────────────────

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

    void ApplyMeshToComponent(USkeletalMeshComponent* Comp,
        USkeletalMesh* Mesh,
        UAnimBlueprint* RetargetABP,
        const TMap<FName, float>& Morphs);

    void SetAllMeshesVisible(bool bVisible);
};