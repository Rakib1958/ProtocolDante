#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NPC_OptimizationComponent.generated.h"

/**
 * Optimization zones based on distance from the player camera.
 * Applied per-NPC every EvaluationInterval seconds.
 *
 * Active   (0–MidZoneDistance)          : Full rate everything
 * Mid      (MidZoneDistance–DistantZone) : Throttled actor + movement, reduced anim update rate
 * Distant  (DistantZoneDistance+)        : Heavy throttle, anim paused, movement ticked minimally
 * Inactive                               : Dead or explicitly disabled, tick off entirely
 */
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

	// ??? Tuning ??????????????????????????????????????????????????????????????

	/** How often (seconds) this component re-evaluates the NPC's distance zone.
	 * Keep this low — 0.5s is imperceptible and saves meaningful evaluations per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization | Timing", meta = (ClampMin = "0.05"))
	float EvaluationInterval = 0.5f;

	/** Distance (units) at which the NPC transitions from Active to Mid zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization | Distances", meta = (ClampMin = "0.0"))
	float MidZoneDistance = 1200.f;

	/** Distance (units) at which the NPC transitions from Mid to Distant zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization | Distances", meta = (ClampMin = "0.0"))
	float DistantZoneDistance = 3000.f;

	// ??? Zone Tick Intervals ?????????????????????????????????????????????????

	/** Actor tick interval in Mid zone (seconds). 0.05 = 20Hz */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float MidZoneActorTickInterval = 0.05f;

	/** Actor tick interval in Distant zone (seconds). 0.1 = 10Hz */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float DistantZoneActorTickInterval = 0.1f;

	/** CharacterMovement tick interval in Mid zone. */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float MidZoneMovementTickInterval = 0.05f;

	/** CharacterMovement tick interval in Distant zone. */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float DistantZoneMovementTickInterval = 0.15f;

	/** LocomotionComponent tick interval in Mid zone. */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float MidZoneLocomotionTickInterval = 0.05f;

	/** LocomotionComponent tick interval in Distant zone. */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Tick Rates")
	float DistantZoneLocomotionTickInterval = 0.2f;

	// ??? Animation LOD ???????????????????????????????????????????????????????

	/** Animation update rate divisor in Mid zone.
	 * 1 = every frame, 2 = every other frame, etc.
	 * URO (Update Rate Optimizations) uses this internally. */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Animation")
	int32 MidZoneAnimUpdateRateDivisor = 2;

	/** Animation update rate divisor in Distant zone. */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Animation")
	int32 DistantZoneAnimUpdateRateDivisor = 4;

	/** Whether to enable mesh visibility-based animation pause.
	 * When true, animation evaluation stops entirely if the mesh is not visible to any camera. */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Animation")
	bool bPauseAnimWhenNotRendered = true;

	/** How many seconds off-screen before animation is paused. */
	UPROPERTY(EditDefaultsOnly, Category = "Optimization | Animation")
	float AnimPauseNotRenderedDelay = 0.5f;

	// ??? State ???????????????????????????????????????????????????????????????

	/** Explicitly Transient to protect runtime calculations from being written to saved archetypes */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Optimization | State")
	EOptimizationZone CurrentZone = EOptimizationZone::Active;

	/** Broadcast whenever the zone changes. Useful for Blueprint cosmetic toggles. */
	UPROPERTY(BlueprintAssignable, Category = "Optimization | State")
	FOnOptimizationZoneChanged OnZoneChanged;

	// ??? Control ?????????????????????????????????????????????????????????????

	/** Call this when the NPC dies or enters a permanently inactive state.
	 * Applies Inactive zone settings and stops the evaluation timer entirely. */
	UFUNCTION(BlueprintCallable, Category = "Optimization")
	void SetInactive();

	/** Instantly forces the NPC back to full active priority status (Zone 0)
	 * and safely flushes/resets the evaluation loop clock tracker. */
	UFUNCTION(BlueprintCallable, Category = "Optimization")
	void ForceWakeup();

	/** Force an immediate zone re-evaluation outside the normal timer interval. */
	UFUNCTION(BlueprintCallable, Category = "Optimization")
	void ForceEvaluateNow();

	UFUNCTION(BlueprintPure, Category = "Optimization")
	EOptimizationZone GetCurrentZone() const { return CurrentZone; }

private:

	// Cached component references — set once in BeginPlay, never looked up again
	UPROPERTY()
	USkeletalMeshComponent* MeshComp = nullptr;

	UPROPERTY()
	UCharacterMovementComponent* MovementComp = nullptr;

	UPROPERTY()
	UActorComponent* LocomotionComp = nullptr;

	FTimerHandle EvaluationTimerHandle;

	UPROPERTY(Transient)
	bool bIsInactive = false;

	// Remembers the original tick interval so we can restore it on zone upgrade
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

	/** Returns squared distance to the nearest local player camera.
	 * Uses camera location, not pawn location, for occlusion-awareness. */
	float GetDistanceSquaredToCamera() const;
};