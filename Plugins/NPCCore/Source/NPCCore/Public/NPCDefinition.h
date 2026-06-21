// NPCDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActivityBase.h"
#include "GoalGeneratorBase.h"
#include "NPCTypes.h"
#include "NPCDefinition.generated.h"

UCLASS()
class NPCCORE_API UNPCDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ── Identity ─────────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
    FName NPCID;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
    FText NPCDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
    ENPCType NPCType = ENPCType::Worker;

    // ── Activities ───────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Activities", Instanced)
    TArray<TObjectPtr<UActivityBase>> Activities;

    // ── Goal Generators ──────────────────────────────────────────────────

    // All generators — reactive ones fired by perception
    // bFireOnInitialize ones fired automatically on Initialize
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Goals", Instanced)
    TArray<TObjectPtr<UGoalGeneratorBase>> GoalGenerators;
};