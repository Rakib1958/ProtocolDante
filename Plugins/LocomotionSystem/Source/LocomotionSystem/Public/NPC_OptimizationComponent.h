#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NPC_OptimizationComponent.generated.h"

UENUM(BlueprintType)
enum class EOptimizationZone : uint8
{
	Active		UMETA(DisplayName = "Active"),
	Mid			UMETA(DisplayName = "Mid"),
	Distant		UMETA(DisplayName = "Distant"),
	Inactive	UMETA(DisplayName = "Inactive")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOptimizationZoneChanged, EOptimizationZone, OldZone, EOptimizationZone, NewZone);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTIONSYSTEM_API UNPC_OptimizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPC_OptimizationComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	// ─── Tuning ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization | Timing", meta = (ClampMin = "0.05"))
	float EvaluationInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization | Distances", meta = (ClampMin = "0.0"))
	float MidZoneDistance = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization | Distances", meta = (ClampMin = "0.0"))
	float DistantZoneDistance = 3000.f;

	// ─── Zone Tick Intervals ─────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float MidZoneActorTickInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float DistantZoneActorTickInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float MidZoneMovementTickInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float DistantZoneMovementTickInterval = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float MidZoneLocomotionTickInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float DistantZoneLocomotionTickInterval = 0.2f;

	// ─── Animation LOD ───────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Animation")
	int32 MidZoneAnimUpdateRateDivisor = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Animation")
	int32 DistantZoneAnimUpdateRateDivisor = 4;

	// ─── State ───────────────────────────────────────────────────────────────

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Optimization | State")
	EOptimizationZone CurrentZone = EOptimizationZone::Active;

	UPROPERTY(BlueprintAssignable, Category = "Optimization | State")
	FOnOptimizationZoneChanged OnZoneChanged;

	// ─── Control ─────────────────────────────────────────────────────────────

	/** Call when the NPC dies. Applies Inactive zone and stops the timer permanently. */
	UFUNCTION(BlueprintCallable, Category = "Optimization")
	void SetInactive();

	/** Force the NPC back to Active zone and restart the evaluation timer.
	 *  Use after knockdown recovery or any state that temporarily froze the NPC. */
	UFUNCTION(BlueprintCallable, Category = "Optimization")
	void ForceWakeup();

	/** Trigger an immediate zone re-evaluation outside the normal timer cadence. */
	UFUNCTION(BlueprintCallable, Category = "Optimization")
	void ForceEvaluateNow();

	UFUNCTION(BlueprintPure, Category = "Optimization")
	EOptimizationZone GetCurrentZone() const { return CurrentZone; }

	/** When dressup meshes are added later, call this from Blueprint after
	 *  all follower mesh anim instances are initialized so the optimization
	 *  component can apply the correct VisibilityBasedAnimTickOption to them.
	 *  No-op until dressup is implemented. */
	UFUNCTION(BlueprintCallable, Category = "Optimization")
	void NotifyFollowerMeshesReady() { /* placeholder for dressup integration */ }

private:

	UPROPERTY()
	USkeletalMeshComponent* MeshComp = nullptr;

	UPROPERTY()
	UCharacterMovementComponent* MovementComp = nullptr;

	UPROPERTY()
	UActorComponent* LocomotionComp = nullptr;

	FTimerHandle EvaluationTimerHandle;

	UPROPERTY(Transient)
	bool bIsInactive = false;

	/** True for locally controlled player character. Timer never starts, always Active. */
	bool bIsPlayerCharacter = false;

	float OriginalActorTickInterval = 0.f;
	float OriginalMovementTickInterval = 0.f;
	float OriginalLocomotionTickInterval = 0.f;

	void EvaluateZone();
	EOptimizationZone CalculateZoneForDistance(float DistanceSquared) const;
	void ApplyZoneSettings(EOptimizationZone NewZone);
	void ApplyActiveZone();
	void ApplyMidZone();
	void ApplyDistantZone();
	void ApplyInactiveZone();
	float GetDistanceSquaredToCamera() const;
};