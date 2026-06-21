#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SignificanceConfig.h"
#include "AC_SignificanceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSignificanceTierChanged, int32, OldTier, int32, NewTier);

UCLASS(ClassGroup = "Optimization", meta = (BlueprintSpawnableComponent))
class OPTIMIZATIONCOMPONENT_API UAC_SignificanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAC_SignificanceComponent();

    // Swapped out the old enum lookup for a flexible configuration string tag
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
    FName NPCTypeTag = FName(TEXT("Worker"));

    UPROPERTY(EditDefaultsOnly, Category = "Configuration")
    TObjectPtr<USignificanceConfig> Config;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSignificanceTierChanged OnSignificanceTierChanged;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    int32 CurrentTier = 2;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsDead = false;

    UPROPERTY(BlueprintReadWrite, Category = "Debug")
    bool bShouldDebug = false;

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void InitializeComponentCustom();

    void EvaluateAndApply(APawn* PlayerPawn, class APlayerCameraManager* CameraManager);
    bool HasActiveOverride() const { return OverrideTimeRemaining > 0.f; }

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void TriggerOverride(float Duration = -1.f);

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void OnOwnerDied();

private:
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> BodyMesh;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponentBudgeted> BudgetedBodyMesh;

    // Hard C++ Decoupling: Store a generic component reference 
    UPROPERTY() TWeakObjectPtr<UActorComponent> CachedMessagingTarget;

    class IAnimationBudgetAllocator* BudgetAllocator = nullptr;
    float OverrideTimeRemaining = 0.f;
    bool bInitialized = false;

    float CalculatePerceivedDistance(APawn* PlayerPawn, APlayerCameraManager* CameraManager) const;
    float CalculateSignificance(APawn* PlayerPawn, APlayerCameraManager* CameraManager) const;
    int32 ResolveToTier(float Significance) const;
    void ApplyTier(int32 NewTier);
};