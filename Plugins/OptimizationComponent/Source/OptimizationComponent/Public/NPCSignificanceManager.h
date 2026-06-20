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
    virtual void Update(TArrayView<const FTransform> InViewpoints) override;

    void RegisterNPC(UAC_SignificanceComponent* Component);
    void UnregisterNPC(UAC_SignificanceComponent* Component);
    void RegisterCorpse(UAC_SignificanceComponent* Component);

    //void SetConfig(USignificanceConfig* InConfig) { Config = InConfig; }

private:
    //UPROPERTY() TObjectPtr<USignificanceConfig> Config;
    UPROPERTY() TArray<TObjectPtr<UAC_SignificanceComponent>> RegisteredComponents;
    UPROPERTY() TArray<TObjectPtr<UAC_SignificanceComponent>> Corpses;

    int32 CurrentBatchIndex = 0;
    float AccumulatedFrameTime = 0.0f;

    void EvaluateBatch(float DeltaSeconds);
    void EvaluateCorpses();

    //float CalculateSignificance(UAC_SignificanceComponent* Component) const;

    //int32 ResolveToTier(UAC_SignificanceComponent* Component, float Significance) const;

    UPROPERTY() TObjectPtr<APawn> CachedPlayerPawn;
    UPROPERTY() TObjectPtr<APlayerCameraManager> CachedCameraManager;
};