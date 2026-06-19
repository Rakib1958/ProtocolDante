#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h" // FIX: Must be included first so the base class definition is fully known
#include "SkeletalMeshComponentBudgeted.h"
#include "NPCTypes.h"
#include "SignificanceConfig.h"
#include "AC_SignificanceComponent.generated.h"

class UAC_NPC_Clothing;
class UMMAnimInstance;
class IAnimationBudgetAllocator;
class APlayerCameraManager;

UCLASS(ClassGroup = "NPC", meta = (BlueprintSpawnableComponent))
class OPTIMIZATIONCOMPONENT_API UAC_SignificanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAC_SignificanceComponent();

    // ── Configuration ────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, Category = "Configuration")
    ENPCType NPCType = ENPCType::Worker;

    UPROPERTY(EditDefaultsOnly, Category = "Configuration")
    TObjectPtr<USignificanceConfig> Config;

    // ── State ────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, Category = "State")
    int32 CurrentTier = 2;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    EAlertState CurrentAlertState = EAlertState::Unaware;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsDead = false;

    // ── Delegates ────────────────────────────────────────────────────────

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTierChanged, int32, OldTier, int32, NewTier);

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTierChanged OnTierChanged;

    // ── Public API ───────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Significance")
    void Initialize(ENPCType InNPCType);

    UFUNCTION(BlueprintCallable, Category = "Significance")
    void SetAlertState(EAlertState NewState);

    UFUNCTION(BlueprintCallable, Category = "Significance")
    void TriggerOverride(float Duration = -1.f);

    UFUNCTION(BlueprintCallable, Category = "Significance")
    void OnOwnerDied();

    // FIX: Aligned signature to accept the camera manager passed from the manager batch loop
    void EvaluateAndApply(APawn* PlayerPawn, APlayerCameraManager* CameraManager);

    // Accessors for manager
    ENPCType GetNPCType() const { return NPCType; }
    EAlertState GetAlertState() const { return CurrentAlertState; }
    bool HasActiveOverride() const { return OverrideTimeRemaining > 0.f; }

    USkeletalMeshComponent* GetBodyMesh() const { return BodyMesh; }

private:
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> BodyMesh;
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponentBudgeted> BudgetedBodyMesh;

    UPROPERTY()
    TObjectPtr<UAC_NPC_Clothing> ClothingComp;

    UPROPERTY()
    TObjectPtr<UMMAnimInstance> AnimInstance;

    IAnimationBudgetAllocator* BudgetAllocator = nullptr;

    float OverrideTimeRemaining = 0.f;
    bool bInitialized = false;

    // ── Internal ─────────────────────────────────────────────────────────

    USkeletalMeshComponent* FindMeshByTag(FName Tag) const;

    // FIX: Declared missing functions and synchronized parameter list setups
    float CalculatePerceivedDistance(APawn* PlayerPawn, APlayerCameraManager* CameraManager) const;
    float CalculateSignificance(APawn* PlayerPawn, APlayerCameraManager* CameraManager) const;
    int32 ResolveToTier(float Significance) const;
    void ApplyTier(int32 NewTier);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};