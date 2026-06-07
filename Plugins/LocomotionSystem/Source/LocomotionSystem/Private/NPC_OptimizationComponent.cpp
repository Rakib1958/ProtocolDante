#include "NPC_OptimizationComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "LocomotionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

UNPC_OptimizationComponent::UNPC_OptimizationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	EvaluationInterval = 0.5f;
	MidZoneDistance = 1200.f;
	DistantZoneDistance = 3000.f;
}

void UNPC_OptimizationComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	// ─── Cache references ─────────────────────────────────────────────────────
	MeshComp = OwnerCharacter->GetMesh();
	MovementComp = OwnerCharacter->GetCharacterMovement();
	LocomotionComp = OwnerCharacter->FindComponentByClass<ULocomotionComponent>();

	OriginalActorTickInterval = OwnerCharacter->PrimaryActorTick.TickInterval;
	OriginalMovementTickInterval = MovementComp ? MovementComp->PrimaryComponentTick.TickInterval : 0.f;
	OriginalLocomotionTickInterval = LocomotionComp ? LocomotionComp->PrimaryComponentTick.TickInterval : 0.f;

	// ─── Player early exit ────────────────────────────────────────────────────
	// Locally controlled character is the player. No timer, no zone evaluation.
	// Player animation always runs at full rate — period.
	if (OwnerCharacter->IsLocallyControlled())
	{
		bIsPlayerCharacter = true;
		if (MeshComp)
		{
			MeshComp->VisibilityBasedAnimTickOption =
				EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		}
		return;
	}

	// ─── NPC path ─────────────────────────────────────────────────────────────
	// Root mesh ticks always. Follower meshes using Copy Pose From Mesh in their
	// anim graph need the root to have evaluated first. VisibilityBasedAnimTickOption
	// on followers is handled separately when dressup is added — NotifyFollowerMeshesReady.
	if (MeshComp)
	{
		MeshComp->bPauseAnims = false;
		MeshComp->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	// ─── Start evaluation timer ───────────────────────────────────────────────
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EvaluationTimerHandle,
			this,
			&UNPC_OptimizationComponent::EvaluateZone,
			EvaluationInterval,
			true
		);
	}

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
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void UNPC_OptimizationComponent::SetInactive()
{
	if (bIsInactive) return;
	bIsInactive = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvaluationTimerHandle);
	}

	ApplyZoneSettings(EOptimizationZone::Inactive);
}

void UNPC_OptimizationComponent::ForceWakeup()
{
	if (bIsInactive || !GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(EvaluationTimerHandle);
	ApplyZoneSettings(EOptimizationZone::Active);
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
	const float MidSq = MidZoneDistance * MidZoneDistance;
	const float DistantSq = DistantZoneDistance * DistantZoneDistance;

	if (DistanceSquared <= MidSq)     return EOptimizationZone::Active;
	if (DistanceSquared <= DistantSq) return EOptimizationZone::Mid;
	return EOptimizationZone::Distant;
}

float UNPC_OptimizationComponent::GetDistanceSquaredToCamera() const
{
	if (!GetOwner()) return 0.f;

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

	OnZoneChanged.Broadcast(OldZone, NewZone);
}

void UNPC_OptimizationComponent::ApplyActiveZone()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	Owner->SetActorTickInterval(OriginalActorTickInterval);
	Owner->SetActorTickEnabled(true);

	if (MovementComp)
	{
		MovementComp->PrimaryComponentTick.TickInterval = OriginalMovementTickInterval;
		MovementComp->SetComponentTickEnabled(true);
	}

	if (LocomotionComp)
	{
		LocomotionComp->PrimaryComponentTick.TickInterval = OriginalLocomotionTickInterval;
		LocomotionComp->SetComponentTickEnabled(true);
	}

	if (MeshComp)
	{
		MeshComp->bEnableUpdateRateOptimizations = false;
	}
}

void UNPC_OptimizationComponent::ApplyMidZone()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	Owner->SetActorTickInterval(MidZoneActorTickInterval);

	if (MovementComp)
	{
		MovementComp->PrimaryComponentTick.TickInterval = MidZoneMovementTickInterval;
	}

	if (LocomotionComp)
	{
		LocomotionComp->PrimaryComponentTick.TickInterval = MidZoneLocomotionTickInterval;
	}

	if (MeshComp)
	{
		MeshComp->bEnableUpdateRateOptimizations = true;
		if (MeshComp->AnimUpdateRateParams)
		{
			MeshComp->AnimUpdateRateParams->MaxEvalRateForInterpolation = MidZoneAnimUpdateRateDivisor;
		}
	}
}

void UNPC_OptimizationComponent::ApplyDistantZone()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	Owner->SetActorTickInterval(DistantZoneActorTickInterval);

	if (MovementComp)
	{
		MovementComp->PrimaryComponentTick.TickInterval = DistantZoneMovementTickInterval;
	}

	if (LocomotionComp)
	{
		LocomotionComp->PrimaryComponentTick.TickInterval = DistantZoneLocomotionTickInterval;
	}

	if (MeshComp)
	{
		MeshComp->bEnableUpdateRateOptimizations = true;
		if (MeshComp->AnimUpdateRateParams)
		{
			MeshComp->AnimUpdateRateParams->MaxEvalRateForInterpolation = DistantZoneAnimUpdateRateDivisor;
		}
		// Root mesh stays AlwaysTickPoseAndRefreshBones (set in BeginPlay).
		// Copy Pose From Mesh in follower anim graphs requires the root to have
		// evaluated. When dressup is added, follower VisibilityBasedAnimTickOption
		// gets its own handling here via NotifyFollowerMeshesReady.
	}
}

void UNPC_OptimizationComponent::ApplyInactiveZone()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	Owner->SetActorTickEnabled(false);

	if (MovementComp)
	{
		MovementComp->SetComponentTickEnabled(false);
		MovementComp->DisableMovement();
	}

	if (LocomotionComp)
	{
		LocomotionComp->SetComponentTickEnabled(false);
	}

	if (MeshComp)
	{
		MeshComp->bEnableUpdateRateOptimizations = false;
		// Ragdoll is physics-driven. Anim evaluation is pointless.
		MeshComp->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		MeshComp->bPauseAnims = true;
	}
}