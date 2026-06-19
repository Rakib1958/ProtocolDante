// SignificanceConfig.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NPCTypes.h" // FIX: Successfully maps ENPCType to your TMap structures
#include "SignificanceConfig.generated.h"

USTRUCT(BlueprintType)
struct FSignificanceTierConfig
{
    GENERATED_BODY()

    // Distance upper bound for this tier
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float MaxDistance = 2000.f;

    // Forced LOD on body mesh (-1 = auto)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 ForcedLOD = -1;

    // Follower mesh tick interval (0 = every frame)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float FollowerTickInterval = 0.f;

    // Locomotion component tick interval (0 = every frame)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float LocomotionTickInterval = 0.f;

    // Cull all meshes entirely
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bCullMeshes = false;

    // Significance float passed to budget allocator (0-1)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float BudgetAllocatorSignificance = 1.f;
};

UCLASS()
class OPTIMIZATIONCOMPONENT_API USignificanceConfig : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Ordered 0 (highest priority) to N (lowest)
    UPROPERTY(EditDefaultsOnly, Category = "Tiers")
    TArray<FSignificanceTierConfig> Tiers;

    // Max distance for WasRecentlyRendered check
    UPROPERTY(EditDefaultsOnly, Category = "Visibility")
    float VisibilityCheckMaxDistance = 6000.f;

    // Time budget per frame for significance evaluation (ms)
    UPROPERTY(EditDefaultsOnly, Category = "Performance")
    float EvaluationBudgetMs = 0.1f;

    // Base tier per NPC type
    UPROPERTY(EditDefaultsOnly, Category = "NPC Types")
    TMap<ENPCType, int32> BaseSignificanceTier;

    // How long event overrides pin to LOD 0
    UPROPERTY(EditDefaultsOnly, Category = "Overrides")
    float EventOverrideDuration = 5.f;

    // Weighted factors for significance scoring
    UPROPERTY(EditDefaultsOnly, Category = "Weights", meta = (ClampMin = 0, ClampMax = 1))
    float DistanceWeight = 0.6f;

    UPROPERTY(EditDefaultsOnly, Category = "Weights", meta = (ClampMin = 0, ClampMax = 1))
    float AlertWeight = 0.3f;

    UPROPERTY(EditDefaultsOnly, Category = "Weights", meta = (ClampMin = 0, ClampMax = 1))
    float TypeWeight = 0.1f;
};