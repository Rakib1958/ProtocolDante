#pragma once

#include "CoreMinimal.h"
#include "SignificanceManager.h"
#include "SignificanceConfig.h"
#include "Tickable.h" // FIX: Required to inherit engine ticking features
#include "NPCSignificanceManager.generated.h"

class UAC_SignificanceComponent;
class APlayerController;
class APawn;
class APlayerCameraManager;

UCLASS()
class OPTIMIZATIONCOMPONENT_API UNPCSignificanceManager : public USignificanceManager, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // ── FTickableGameObject Interface Hooks ──────────────────────────────
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; } // Prevent ticking inside raw asset windows
    // ─────────────────────────────────────────────────────────────────────

    virtual void Update(TArrayView<const FTransform> InViewpoints) override;

    void RegisterNPC(UAC_SignificanceComponent* Component);
    void UnregisterNPC(UAC_SignificanceComponent* Component);
    void RegisterCorpse(UAC_SignificanceComponent* Component);

private:
    UPROPERTY()
    TArray<TObjectPtr<UAC_SignificanceComponent>> RegisteredComponents;

    UPROPERTY()
    TArray<TObjectPtr<UAC_SignificanceComponent>> Corpses;

    int32 CurrentBatchIndex = 0;
    float AccumulatedFrameTime = 0.0f;

    void EvaluateBatch(float DeltaSeconds);
    void EvaluateCorpses();

    UPROPERTY()
    TObjectPtr<APawn> CachedPlayerPawn;

    UPROPERTY()
    TObjectPtr<APlayerCameraManager> CachedCameraManager;
};