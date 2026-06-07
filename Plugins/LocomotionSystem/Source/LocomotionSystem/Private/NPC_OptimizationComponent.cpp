#include "NPC_OptimizationComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "LocomotionComponent.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

UNPC_OptimizationComponent::UNPC_OptimizationComponent()
{
	// This component runs only its timer — no per-frame tick needed.
	// All NPC throttling is driven by the EvaluationInterval timer.
	PrimaryComponentTick.bCanEverTick = false;

	// Configurable default values matching our updated header thresholds
	EvaluationInterval = 0.5f;
	MidZoneDistance = 1200.f;
	DistantZoneDistance = 3000.f;
}

void UNPC_OptimizationComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	// ─── Cache component references once ─────────────────────────────────────
	MeshComp = OwnerCharacter->GetMesh();
	MovementComp = OwnerCharacter->GetCharacterMovement();
	LocomotionComp = OwnerCharacter->FindComponentByClass<ULocomotionComponent>();

	// ─── Capture original tick intervals before we modify anything ───────────
	// These are the "full rate" baselines we restore when entering Active zone.
	OriginalActorTickInterval = OwnerCharacter->PrimaryActorTick.TickInterval;
	OriginalMovementTickInterval = MovementComp ? MovementComp->PrimaryComponentTick.TickInterval : 0.f;
	OriginalLocomotionTickInterval = LocomotionComp ? LocomotionComp->PrimaryComponentTick.TickInterval : 0.f;

	// ─── Animation visibility pause ──────────────────────────────────────────
	if (MeshComp && bPauseAnimWhenNotRendered)
	{
		// UE5 built-in: stop animating when the mesh hasn't been rendered for N seconds.
		// This handles occlusion and off-screen NPCs at zero cost.
		MeshComp->bPauseAnims = false; // we control this ourselves via URO or Inactive states
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}

	// ─── Start evaluation timer ───────────────────────────────────────────────
	// Timer fires every EvaluationInterval seconds, re-checks the distance zone,
	// and applies throttling only if the zone has changed. Cheap.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EvaluationTimerHandle,
			this,
			&UNPC_OptimizationComponent::EvaluateZone,
			EvaluationInterval,
			true  // looping
		);
	}

	// ─── Run one evaluation immediately so we don't start at wrong defaults ──
	EvaluateZone();
}

void UNPC_OptimizationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvaluationTimerHandle);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API / Control
// ─────────────────────────────────────────────────────────────────────────────

void UNPC_OptimizationComponent::SetInactive()
{
	if (bIsInactive) return;
	bIsInactive = true;

	// Stop the evaluation timer — a dead or inactive NPC never needs re-evaluation.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvaluationTimerHandle);
	}

	ApplyZoneSettings(EOptimizationZone::Inactive);
}

void UNPC_OptimizationComponent::ForceWakeup()
{
	// A dead/permanently inactive NPC mesh shouldn't be forced awake by game systems
	if (bIsInactive || !GetWorld()) return;

	// Clear and reset the running clock cycle so an imminent evaluation loop pass 
	// doesn't instantly drop our actor back into a sleeping frame tier
	GetWorld()->GetTimerManager().ClearTimer(EvaluationTimerHandle);

	// Force-restore maximum frame and logic processing rates immediately
	ApplyZoneSettings(EOptimizationZone::Active);

	// Re-initialize the running evaluation clock loop interval safely
	GetWorld()->GetTimerManager().SetTimer(
		EvaluationTimerHandle,
		this,
		&UNPC_OptimizationComponent::EvaluateZone,
		EvaluationInterval,
		true
	);
}

void UNPC_OptimizationComponent::ForceEvaluateNow()
{
	if (!bIsInactive)
	{
		EvaluateZone();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Core Evaluation
// ─────────────────────────────────────────────────────────────────────────────

void UNPC_OptimizationComponent::EvaluateZone()
{
	if (bIsInactive) return;

	const float DistSq = GetDistanceSquaredToCamera();
	const EOptimizationZone NewZone = CalculateZoneForDistance(DistSq);

	if (NewZone != CurrentZone)
	{
		ApplyZoneSettings(NewZone);
	}
}

EOptimizationZone UNPC_OptimizationComponent::CalculateZoneForDistance(float DistanceSquared) const
{
	// Compare squared distances to avoid sqrt every evaluation tick
	const float MidSq = MidZoneDistance * MidZoneDistance;
	const float DistantSq = DistantZoneDistance * DistantZoneDistance;

	if (DistanceSquared <= MidSq)     return EOptimizationZone::Active;
	if (DistanceSquared <= DistantSq) return EOptimizationZone::Mid;
	return EOptimizationZone::Distant;
}

float UNPC_OptimizationComponent::GetDistanceSquaredToCamera() const
{
	if (!GetOwner()) return 0.f;

	// Use the local player's camera location, not the pawn location.
	// This correctly handles cinematic cameras, cutscenes, and ADS zoom.
	const APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return 0.f;

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	return FVector::DistSquared(GetOwner()->GetActorLocation(), CameraLocation);
}

// ─────────────────────────────────────────────────────────────────────────────
// Zone Application
// ─────────────────────────────────────────────────────────────────────────────

void UNPC_OptimizationComponent::ApplyZoneSettings(EOptimizationZone NewZone)
{
	const EOptimizationZone OldZone = CurrentZone;
	CurrentZone = NewZone;

	switch (NewZone)
	{
	case EOptimizationZone::Active:   ApplyActiveZone();   break;
	case EOptimizationZone::Mid:      ApplyMidZone();      break;
	case EOptimizationZone::Distant:  ApplyDistantZone();  break;
	case EOptimizationZone::Inactive: ApplyInactiveZone(); break;
	}

	// Notify Blueprint — useful for toggling cosmetic systems (shadow casting,
	// decal receivers, particle effects) per zone without C++ changes.
	OnZoneChanged.Broadcast(OldZone, NewZone);
}

void UNPC_OptimizationComponent::ApplyActiveZone()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// ─── Actor tick: restore full rate ───────────────────────────────────────
	Owner->SetActorTickInterval(OriginalActorTickInterval);
	Owner->SetActorTickEnabled(true);

	// ─── CharacterMovement: restore full rate ─────────────────────────────────
	if (MovementComp)
	{
		MovementComp->PrimaryComponentTick.TickInterval = OriginalMovementTickInterval;
		MovementComp->SetComponentTickEnabled(true);
	}

	// ─── LocomotionComponent: restore full rate ───────────────────────────────
	if (LocomotionComp)
	{
		LocomotionComp->PrimaryComponentTick.TickInterval = OriginalLocomotionTickInterval;
		LocomotionComp->SetComponentTickEnabled(true);
	}

	// ─── Animation: full update rate, respect visibility pause setting ────────
	if (MeshComp)
	{
		MeshComp->bEnableUpdateRateOptimizations = false;
		// Re-enable OnlyTickPoseWhenRendered if configured — still free occlusion culling
		if (bPauseAnimWhenNotRendered)
		{
			MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}
	}
}

void UNPC_OptimizationComponent::ApplyMidZone()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// ─── Actor tick: throttle to ~20Hz ───────────────────────────────────────
	Owner->SetActorTickInterval(MidZoneActorTickInterval);

	// ─── CharacterMovement: throttle to ~20Hz ────────────────────────────────
	// Movement still runs — NPC is patrolling and needs physics/nav integration.
	if (MovementComp)
	{
		MovementComp->PrimaryComponentTick.TickInterval = MidZoneMovementTickInterval;
	}

	// ─── LocomotionComponent: throttle ───────────────────────────────────────
	if (LocomotionComp)
	{
		LocomotionComp->PrimaryComponentTick.TickInterval = MidZoneLocomotionTickInterval;
	}

	// ─── Animation: URO divisor — animate every Nth frame ───────────────────
	if (MeshComp)
	{
		MeshComp->bEnableUpdateRateOptimizations = true;

		// Defensive validation check to prevent null-ptr exceptions if URO parameters aren't initialized yet
		if (MeshComp->AnimUpdateRateParams)
		{
			MeshComp->AnimUpdateRateParams->MaxEvalRateForInterpolation = MidZoneAnimUpdateRateDivisor;
		}

		if (bPauseAnimWhenNotRendered)
		{
			MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}
	}
}

void UNPC_OptimizationComponent::ApplyDistantZone()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// ─── Actor tick: throttle to ~10Hz ───────────────────────────────────────
	Owner->SetActorTickInterval(DistantZoneActorTickInterval);

	// ─── CharacterMovement: heavy throttle ───────────────────────────────────
	if (MovementComp)
	{
		MovementComp->PrimaryComponentTick.TickInterval = DistantZoneMovementTickInterval;
	}

	// ─── LocomotionComponent: heavy throttle ─────────────────────────────────
	if (LocomotionComp)
	{
		LocomotionComp->PrimaryComponentTick.TickInterval = DistantZoneLocomotionTickInterval;
	}

	// ─── Animation: high URO divisor, pose interpolation handles the gap ─────
	if (MeshComp)
	{
		MeshComp->bEnableUpdateRateOptimizations = true;

		if (MeshComp->AnimUpdateRateParams)
		{
			MeshComp->AnimUpdateRateParams->MaxEvalRateForInterpolation = DistantZoneAnimUpdateRateDivisor;
		}

		// At this distance, full visibility-based pause is appropriate.
		// If the NPC isn't on screen, don't animate at all.
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		if (bPauseAnimWhenNotRendered)
		{
			MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}
	}
}

void UNPC_OptimizationComponent::ApplyInactiveZone()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// ─── Actor tick: off entirely ─────────────────────────────────────────────
	Owner->SetActorTickEnabled(false);

	// ─── CharacterMovement: off ───────────────────────────────────────────────
	if (MovementComp)
	{
		MovementComp->SetComponentTickEnabled(false);
		MovementComp->DisableMovement(); // Stops CMC from fighting physics sim
	}

	// ─── LocomotionComponent: off ─────────────────────────────────────────────
	if (LocomotionComp)
	{
		LocomotionComp->SetComponentTickEnabled(false);
	}

	// ─── Animation: let physics drive the ragdoll, stop anim evaluation ───────
	if (MeshComp)
	{
		MeshComp->bEnableUpdateRateOptimizations = false;
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

		// Stop the animation blueprint from evaluating entirely.
		// The ragdoll is physics-driven; there is no pose to evaluate.
		MeshComp->bPauseAnims = true;
	}
}