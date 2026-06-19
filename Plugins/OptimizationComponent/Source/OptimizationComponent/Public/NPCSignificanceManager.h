#pragma once

#include "CoreMinimal.h"
#include "SignificanceManager.h"
#include "SignificanceConfig.h"
#include "NPCSignificanceManager.generated.h"

class UAC_SignificanceComponent;
class APlayerController;
class APawn;
class APlayerCameraManager;

UCLASS()
class OPTIMIZATIONCOMPONENT_API UNPCSignificanceManager : public USignificanceManager
{
    GENERATED_BODY()

public:
    // FIX: Removed BeginPlay and updated Update to match the true engine viewpoints signature
    virtual void Update(TArrayView<const FTransform> InViewpoints) override;

    // Called by AC_SignificanceComponent on NPC BeginPlay
    void RegisterNPC(UAC_SignificanceComponent* Component);
    void UnregisterNPC(UAC_SignificanceComponent* Component);

    void SetConfig(USignificanceConfig* InConfig) { Config = InConfig; }

private:
    UPROPERTY()
    TObjectPtr<USignificanceConfig> Config;

    // All registered NPC significance components
    UPROPERTY()
    TArray<TObjectPtr<UAC_SignificanceComponent>> RegisteredComponents;

    // Staggered evaluation state
    int32 CurrentBatchIndex = 0;
    float AccumulatedFrameTime = 0.0f;

    // FIX: Changed signature to take DeltaSeconds from world context
    void EvaluateBatch(float DeltaSeconds);

    // FIX: Declared missing cleanup function
    void EvaluateCorpses();

    // Returns significance float 0-1 for a single NPC
    float CalculateSignificance(UAC_SignificanceComponent* Component) const;

    // Resolves significance float to a tier index
    int32 ResolveToTier(UAC_SignificanceComponent* Component, float Significance) const;

    // FIX: Cleaned up dynamic runtime objects with correct properties
    UPROPERTY()
    TObjectPtr<APawn> CachedPlayerPawn;

    UPROPERTY()
    TObjectPtr<APlayerCameraManager> CachedCameraManager;
};