#include "LocomotionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Engine/Engine.h"

ULocomotionComponent::ULocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	SetReferences();
}

void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsValidCharacter) return;
	if (MovementMode == Enum_MovementMode::Ragdoll) return;

	// 1. Process capsule smoothing universally across ALL stance changes
	TickStanceTransition(DeltaTime);
	TickFloorDetection(DeltaTime);

	// 2. Fire multi-cast handlers to let player Blueprint update base tracking matrices
	OnUpdateMovement.Broadcast();
	OnUpdateRotation.Broadcast();

	// ─── FIX: ENFORCE PRONE ROTATION OVERRIDES AFTER BLUEPRINT DELEGATE TUPLES ───
	if (Stance == Enum_Stance::Prone)
	{
		Character->bUseControllerRotationYaw = false;
		float CurrentYaw = CharacterMovement->RotationRate.Yaw;
		float TargetYaw = ProneRotationRateYaw;
		CharacterMovement->RotationRate = FRotator(
			0.f,
			FMath::FInterpTo(CurrentYaw < 0.f ? 360.f : CurrentYaw, TargetYaw, DeltaTime, 8.f),
			0.f
		);
		if (!CurrentMeshSlopeOffset.IsNearlyZero(0.1f))
		{
			TickProneAlignment(DeltaTime);
		}

	}
}

void ULocomotionComponent::SetReferences()
{
	Character = Cast<ACharacter>(GetOwner());
	if (IsValid(Character))
	{
		CharacterMovement = Character->GetCharacterMovement();
		CapsuleComponent = Character->GetCapsuleComponent();
		Mesh = Character->GetMesh();
		MainAnimInstance = Mesh->GetAnimInstance();

		if (IsValid(CharacterMovement) && IsValid(CapsuleComponent) && IsValid(Mesh) && IsValid(MainAnimInstance))
		{
			bIsValidCharacter = true;

			// Initialize clean bounding configurations
			CapsuleComponent->SetCapsuleHalfHeight(StandCapsuleHalfHeight);
			CharacterMovement->CrouchedHalfHeight = CrouchCapsuleHalfHeight;
			TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
		}
	}
}

// ─── STANCE INTERPOLATION CORE ───
void ULocomotionComponent::TickStanceTransition(float DeltaTime)
{
	float CurrentHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();

	// 1. CAPSULE RESIZING (Runs completely un-gated for Stand, Crouch, and Prone transitions)
	if (!FMath::IsNearlyEqual(CurrentHalfHeight, TargetCapsuleHalfHeight, 0.05f))
	{
		float NewHalfHeight = FMath::FInterpTo(CurrentHalfHeight, TargetCapsuleHalfHeight, DeltaTime, StanceTransitionSpeed);
		float HalfHeightDelta = CurrentHalfHeight - NewHalfHeight;

		// FIX: Set bSweep to false to eliminate world physics grounding stutter and let it drop fully
		FVector NewWorldLocation = Character->GetActorLocation();
		NewWorldLocation.Z -= HalfHeightDelta;
		Character->SetActorLocation(NewWorldLocation, false);

		CapsuleComponent->SetCapsuleHalfHeight(NewHalfHeight, true);
	}

	// 2. DYNAMIC PRONE FLOOR COMPENSATOR (Prone Idle Offset)
	float TargetIdleOffset = 0.f;
	if (Stance == Enum_Stance::Prone)
	{
		bool bIsProneIdle = CharacterMovement->Velocity.Size2D() < 10.f;
		TargetIdleOffset = bIsProneIdle ? ProneIdleMeshZOffset : 0.f;
	}

	CurrentProneIdleOffset = FMath::FInterpTo(CurrentProneIdleOffset, TargetIdleOffset, DeltaTime, StanceTransitionSpeed);

	// 3. FINAL COORDINATE SYNC
	// Anchors mesh origin directly to the bottom of the capsule across all layout positions
	//float FinalMeshZ = -CapsuleComponent->GetUnscaledCapsuleHalfHeight() + CurrentProneIdleOffset;
	float FinalMeshZ = -CapsuleComponent->GetUnscaledCapsuleHalfHeight() + CurrentProneIdleOffset + FootPlacementRootOffset;
	Mesh->SetRelativeLocation(FVector(Mesh->GetRelativeLocation().X, Mesh->GetRelativeLocation().Y, FinalMeshZ));
}

void ULocomotionComponent::TickProneLedgeCheck(float DeltaTime)
{
	if (Stance != Enum_Stance::Prone)
	{
		ProneSettleTimer = 0.f; // reset when leaving prone
		return;
	}
	if (MovementMode == Enum_MovementMode::Ragdoll) return;

	// Accumulate settle time
	ProneSettleTimer += DeltaTime;
	if (ProneSettleTimer < ProneSettleDelay) return; // not settled yet

	if (CharacterMovement->Velocity.Size2D() < 5.f) return;

	// Probe from chest height forward in movement direction
	FVector MoveDir = CharacterMovement->Velocity.GetSafeNormal2D();
	if (MoveDir.IsNearlyZero()) return;

	FVector ChestLocation = Character->GetActorLocation()
		+ MoveDir * LedgeCheckDistance
		+ FVector(0.f, 0.f, 10.f); // slight upward offset from actor origin

	FVector TraceEnd = ChestLocation - FVector(0.f, 0.f, LedgeCheckDepth);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	bool bGroundAhead = Character->GetWorld()->LineTraceSingleByChannel(
		Hit, ChestLocation, TraceEnd, ECC_Visibility, Params);

	if (!bGroundAhead)
	{
		StartRagdoll();
	}
}

void ULocomotionComponent::TickProneAlignment(float DeltaTime)
{
	if (Stance != Enum_Stance::Prone)
	{
		// Smoothly reset pitch/roll when leaving prone
		if (!CurrentMeshSlopeOffset.IsNearlyZero(0.1f))
		{
			CurrentMeshSlopeOffset = FMath::RInterpTo(
				CurrentMeshSlopeOffset, FRotator::ZeroRotator, DeltaTime, SlopeAlignmentInterpSpeed
			);
			FRotator Current = Mesh->GetRelativeRotation();
			Mesh->SetRelativeRotation(FRotator(
				CurrentMeshSlopeOffset.Pitch,
				Current.Yaw,
				CurrentMeshSlopeOffset.Roll
			));
		}
		return;
	}

	FVector Start = Character->GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, SlopeTraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	FRotator TargetSlopeRot = FRotator::ZeroRotator;

	if (Character->GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params))
	{
		FVector FloorNormal = Hit.ImpactNormal;

		// Use velocity direction when moving, actor forward when idle
		bool bMoving = CharacterMovement->Velocity.Size2D() > 10.f;
		FVector RefForward = bMoving
			? CharacterMovement->Velocity.GetSafeNormal2D()
			: Character->GetActorForwardVector();

		FVector Right = FVector::CrossProduct(FloorNormal, RefForward).GetSafeNormal();
		FVector AdjForward = FVector::CrossProduct(Right, FloorNormal).GetSafeNormal();

		FMatrix AlignMatrix(AdjForward, Right, FloorNormal, FVector::ZeroVector);
		FRotator AlignRot = AlignMatrix.Rotator();

		// Only take pitch and roll
		TargetSlopeRot = FRotator(AlignRot.Pitch, 0.f, AlignRot.Roll);
	}

	CurrentMeshSlopeOffset = FMath::RInterpTo(
		CurrentMeshSlopeOffset, TargetSlopeRot, DeltaTime, SlopeAlignmentInterpSpeed
	);

	// Just expose — Transform Modify Bone in ABP reads these
	ProneSlopePitch = CurrentMeshSlopeOffset.Pitch;
	ProneSlopeRoll = CurrentMeshSlopeOffset.Roll;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Hello")));
	}
}

void ULocomotionComponent::TickFloorDetection(float DeltaTime)
{
	FVector Start = Character->GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 50.f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	if (Character->GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params))
	{
		float RawAngle = FMath::RadiansToDegrees(
			FMath::Acos(FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector))
		);

		// Smooth the angle so pelvis doesn't snap on terrain transitions
		FloorAngle = FMath::FInterpTo(FloorAngle, RawAngle, DeltaTime, 6.f);
	}
	else
	{
		FloorAngle = FMath::FInterpTo(FloorAngle, 0.f, DeltaTime, 6.f);
	}

	// Flat(0°) → MaxOffset=250, Steep(30°+) → MaxOffset=0
	DynamicPelvisMaxOffset = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, 30.f),
		FVector2D(250.f, 0.f),
		FloorAngle
	);

	// Flat → normal stiffness, Steep → high stiffness for snappy small corrections
	DynamicPelvisStiffness = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, 30.f),
		FVector2D(100.f, 400.f),
		FloorAngle
	);
}

// ─── QUERIES ───
bool ULocomotionComponent::HasMovementInputVector()
{
	return bIsValidCharacter ? UKismetMathLibrary::NotEqual_VectorVector(Character->GetPendingMovementInputVector(), FVector::ZeroVector, 0.f) : false;
}

bool ULocomotionComponent::CanSprint() const
{
	if (!bIsValidCharacter) return false;

	return CharacterInputState.WantsToRun &&
		(CharacterMovement->bOrientRotationToMovement ||
			((UKismetMathLibrary::NormalizedDeltaRotator(Character->GetActorRotation(),
				UKismetMathLibrary::MakeRotFromX(Character->IsLocallyControlled() ? Character->GetPendingMovementInputVector() : CharacterMovement->GetCurrentAcceleration())).Yaw) < 45.f));
}

bool ULocomotionComponent::IsSprinting()
{
	return CharacterInputState.WantsToRun && CharacterMovement->Velocity.SizeSquared2D() > 0.f;
}

bool ULocomotionComponent::CanStandFromProne() const
{
	if (!bIsValidCharacter) return false;

	// Evaluate clear ceiling space up to the Crouch Capsule Half Height limit profile
	float TraceDistance = CrouchCapsuleHalfHeight - ProneCapsuleHalfHeight;
	FVector Start = Character->GetActorLocation();
	FVector End = Start + FVector(0.f, 0.f, TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	return !Character->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}

// ─── EXECUTION REFRESHERS ───
void ULocomotionComponent::UpdateGait()
{
	if (GetDesiredGait() != Gait) Gait = GetDesiredGait();
}

void ULocomotionComponent::UpdateDynamicMovementSettings()
{
	if (!bIsValidCharacter) return;

	CharacterMovement->MaxAcceleration = CalculateMaxAcceleration();
	CharacterMovement->BrakingDecelerationWalking = CalculateMaxBrakingDeceleration();
	CharacterMovement->GroundFriction = CalculateGroundFriction();

	CharacterMovement->MaxWalkSpeed = (Stance == Enum_Stance::Prone) ? CalculateMaxSpeedProned() : CalculateMaxSpeed();
	CharacterMovement->MaxWalkSpeedCrouched = (Stance == Enum_Stance::Prone) ? CalculateMaxSpeedProned() : CalculateMaxSpeedCrouched();
}

// ─── STATE ARCHITECT MATRIX ───
void ULocomotionComponent::SetStance(Enum_Stance NewStance)
{
	if (Stance == NewStance) return;
	Stance = NewStance;

	// FIX: Synchronize internal intent state so native checks pass perfectly on the first press
	if (CharacterMovement)
	{
		CharacterMovement->bWantsToCrouch = (Stance == Enum_Stance::Crouch);
	}

	switch (Stance)
	{
	case Enum_Stance::Stand:  TargetCapsuleHalfHeight = StandCapsuleHalfHeight;  break;
	case Enum_Stance::Crouch: TargetCapsuleHalfHeight = CrouchCapsuleHalfHeight; break;
	case Enum_Stance::Prone:  TargetCapsuleHalfHeight = ProneCapsuleHalfHeight;  break;
	}

	UpdateDynamicMovementSettings();
}

void ULocomotionComponent::SetMovementMode(Enum_MovementMode NewMovementMode)
{
	if (MovementMode != NewMovementMode)
	{
		MovementMode = NewMovementMode;
		UpdateDynamicMovementSettings();
	}
}

void ULocomotionComponent::SetCharacterState(Enum_CharacterState NewCharacterState)
{
	if (CharacterState != NewCharacterState) CharacterState = NewCharacterState;
}

void ULocomotionComponent::SetProne(bool bWantsToProne)
{
	if (!bIsValidCharacter) return;

	if (bWantsToProne)
	{
		SetStance(Enum_Stance::Prone);
		CharacterInputState.WantsToStrafe = true;
	}
	else
	{
		if (!CanStandFromProne()) return;
		SetStance(Enum_Stance::Crouch);
		CharacterInputState.WantsToStrafe = false;
	}
}

// ─── SPEED & MATHEMATICAL EVALUATORS ───
float ULocomotionComponent::CalculateMaxSpeedProned()
{
	if (!IsValid(StrafeSpeedMapCurve)) return ProneSpeed.X;

	float StrafeSpeedMap = StrafeSpeedMapCurve->GetFloatValue(FMath::Abs(UKismetAnimationLibrary::CalculateDirection(CharacterMovement->Velocity, Character->GetActorRotation())));

	return StrafeSpeedMap < 1.f ? UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.f, ProneSpeed.X, ProneSpeed.Y) : UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.f, ProneSpeed.Y, ProneSpeed.Z);
}

float ULocomotionComponent::CalculateGroundFriction()
{
	if (Stance == Enum_Stance::Prone) return ProneGroundFriction;

	if (Gait == Enum_Gait::Jog || Gait == Enum_Gait::Walk) return WalkGroundFriction;
	return UKismetMathLibrary::MapRangeClamped(CharacterMovement->Velocity.Size2D(), 0.f, RunSpeed.X, WalkGroundFriction, RunGroundFriction);
}

float ULocomotionComponent::CalculateMaxAcceleration()
{
	if (Gait == Enum_Gait::Walk || Gait == Enum_Gait::Jog) return WalkAcceleration;
	return UKismetMathLibrary::MapRangeClamped(CharacterMovement->Velocity.Size2D(), WalkSpeedRelaxed.X * 1.35, RunSpeed.X, WalkAcceleration, RunAcceleration);
}

float ULocomotionComponent::CalculateMaxBrakingDeceleration()
{
	return HasMovementInputVector() ? BrakingDeceleration : 2000.f;
}

float ULocomotionComponent::CalculateMaxSpeed()
{
	if (!IsValid(StrafeSpeedMapCurve)) return JogSpeed.X;

	float StrafeSpeedMap = CharacterMovement->bUseControllerDesiredRotation ? StrafeSpeedMapCurve->GetFloatValue(abs(UKismetAnimationLibrary::CalculateDirection(CharacterMovement->Velocity, Character->GetActorRotation()))) : 0.f;
	FVector DesiredSpeed = FVector::ZeroVector;

	switch (Gait)
	{
	case Enum_Gait::Walk:
		switch (CharacterState)
		{
		case Enum_CharacterState::Relaxed: DesiredSpeed = WalkSpeedRelaxed; break;
		case Enum_CharacterState::Stealth: DesiredSpeed = WalkSpeedStealth; break;
		case Enum_CharacterState::Combat:  DesiredSpeed = WalkSpeedCombat;  break;
		}
		break;
	case Enum_Gait::Run: DesiredSpeed = RunSpeed; break;
	case Enum_Gait::Jog: DesiredSpeed = JogSpeed; break;
	}

	return StrafeSpeedMap < 1.f ? UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.f, DesiredSpeed.X, DesiredSpeed.Y) : UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.f, DesiredSpeed.Y, DesiredSpeed.Z);
}

float ULocomotionComponent::CalculateMaxSpeedCrouched()
{
	if (!IsValid(StrafeSpeedMapCurve)) return CrouchSpeed.X;

	float StrafeSpeedMap = CharacterMovement->bOrientRotationToMovement ? 0.f : StrafeSpeedMapCurve->GetFloatValue(FMath::Abs(UKismetAnimationLibrary::CalculateDirection(CharacterMovement->Velocity, Character->GetActorRotation())));
	return StrafeSpeedMap < 1.f ? UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.f, CrouchSpeed.X, CrouchSpeed.Y) : UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.f, CrouchSpeed.Y, CrouchSpeed.Z);
}

Enum_Gait ULocomotionComponent::GetDesiredGait()
{
	if (CanSprint()) return Enum_Gait::Run;
	if (CharacterInputState.WantsToWalk) return Enum_Gait::Walk;
	return Enum_Gait::Jog;
}

// ─── NATIVE HOOK RUNTIMES ───
void ULocomotionComponent::WantsToStrafe_Implementation() { CharacterInputState.WantsToStrafe = !CharacterInputState.WantsToStrafe; }
void ULocomotionComponent::WantsToRun_Implementation(bool bStarted) { CharacterInputState.WantsToRun = bStarted; }
void ULocomotionComponent::WantsToAim_Implementation(bool bStarted) { CharacterInputState.WantsToAim = bStarted; }
void ULocomotionComponent::WantsToWalk_Implementation(bool bStarted) { CharacterInputState.WantsToWalk = bStarted; }