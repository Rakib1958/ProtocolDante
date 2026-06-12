#include "LocomotionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"

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

	// Smooth capsule height transitions for all stance changes
	TickStanceTransition(DeltaTime);

	OnUpdateMovement.Broadcast();
	OnUpdateRotation.Broadcast();

	// Prone rotation override — runs after delegates so it wins over GASP's -1 rate
	if (Stance == Enum_Stance::Prone)
	{
		Character->bUseControllerRotationYaw = false;
		float CurrentYaw = CharacterMovement->RotationRate.Yaw;
		// Guard against -1 (infinite) rate — interp from a sane value instead
		float FromYaw = (CurrentYaw < 0.f) ? 360.f : CurrentYaw;
		CurrentProneRotationRate = FMath::FInterpTo(FromYaw, ProneRotationRate, DeltaTime, ProneRotationInterpSpeed);
		CharacterMovement->RotationRate = FRotator(0.f, CurrentProneRotationRate, 0.f);
	}
}

// ── References ────────────────────────────────────────────────────────────────

void ULocomotionComponent::SetReferences()
{
	Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character)) return;

	CharacterMovement = Character->GetCharacterMovement();
	CapsuleComponent = Character->GetCapsuleComponent();
	Mesh = Character->GetMesh();
	MainAnimInstance = Mesh->GetAnimInstance();

	if (IsValid(CharacterMovement) && IsValid(CapsuleComponent) && IsValid(Mesh) && IsValid(MainAnimInstance))
	{
		bIsValidCharacter = true;
		// Sync capsule to stand height on init
		CapsuleComponent->SetCapsuleHalfHeight(StandCapsuleHalfHeight);
		CharacterMovement->CrouchedHalfHeight = CrouchCapsuleHalfHeight;
		TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
	}
}

// ── Stance Transition ─────────────────────────────────────────────────────────

void ULocomotionComponent::TickStanceTransition(float DeltaTime)
{
	float Current = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	if (FMath::IsNearlyEqual(Current, TargetCapsuleHalfHeight, 0.1f)) return;

	float NewHeight = FMath::FInterpTo(Current, TargetCapsuleHalfHeight, DeltaTime, StanceTransitionSpeed);
	float HeightDelta = Current - NewHeight;

	// Move actor down as capsule shrinks so feet stay grounded
	FVector Loc = Character->GetActorLocation();
	Loc.Z -= HeightDelta;
	Character->SetActorLocation(Loc, false);

	CapsuleComponent->SetCapsuleHalfHeight(NewHeight, true);

	// Anchor mesh bottom to capsule bottom — fixes idle mesh drop
	// Mesh relative Z should always be -CapsuleHalfHeight
	FVector MeshRelLoc = Mesh->GetRelativeLocation();
	MeshRelLoc.Z = -NewHeight;
	Mesh->SetRelativeLocation(MeshRelLoc);
}

// ── Queries ───────────────────────────────────────────────────────────────────

bool ULocomotionComponent::HasMovementInputVector()
{
	return bIsValidCharacter
		? UKismetMathLibrary::NotEqual_VectorVector(Character->GetPendingMovementInputVector(), FVector::ZeroVector, 0.f)
		: false;
}

bool ULocomotionComponent::CanSprint() const
{
	if (!bIsValidCharacter) return false;
	if (Stance != Enum_Stance::Stand) return false; // no sprinting while crouched or prone

	return CharacterInputState.WantsToRun &&
		(CharacterMovement->bOrientRotationToMovement ||
			(UKismetMathLibrary::NormalizedDeltaRotator(
				Character->GetActorRotation(),
				UKismetMathLibrary::MakeRotFromX(
					Character->IsLocallyControlled()
					? Character->GetPendingMovementInputVector()
					: CharacterMovement->GetCurrentAcceleration()
				)).Yaw < 45.f));
}

bool ULocomotionComponent::IsSprinting()
{
	return CharacterInputState.WantsToRun
		&& CharacterMovement->Velocity.SizeSquared2D() > FMath::Square(JogSpeed.X);
}

bool ULocomotionComponent::CanStandFromProne() const
{
	if (!bIsValidCharacter) return false;

	// Only need clearance up to crouch height from prone height
	float TraceDistance = CrouchCapsuleHalfHeight - ProneCapsuleHalfHeight;
	FVector Start = Character->GetActorLocation();
	FVector End = Start + FVector(0.f, 0.f, TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	return !Character->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}

// ── Update ────────────────────────────────────────────────────────────────────

void ULocomotionComponent::UpdateGait()
{
	Enum_Gait Desired = GetDesiredGait();
	if (Desired != Gait) Gait = Desired;
}

void ULocomotionComponent::UpdateDynamicMovementSettings()
{
	if (!bIsValidCharacter) return;

	CharacterMovement->MaxAcceleration = CalculateMaxAcceleration();
	CharacterMovement->BrakingDecelerationWalking = CalculateMaxBrakingDeceleration();
	CharacterMovement->GroundFriction = CalculateGroundFriction();

	if (Stance == Enum_Stance::Prone)
	{
		// Prone uses MaxWalkSpeed regardless — character is never in CMC's crouched state
		CharacterMovement->MaxWalkSpeed = CalculateMaxSpeedProne();
		CharacterMovement->MaxWalkSpeedCrouched = CalculateMaxSpeedProne();
	}
	else
	{
		CharacterMovement->MaxWalkSpeed = CalculateMaxSpeed();
		CharacterMovement->MaxWalkSpeedCrouched = CalculateMaxSpeedCrouched();
	}
}

// ── Set State ─────────────────────────────────────────────────────────────────

void ULocomotionComponent::SetStance(Enum_Stance NewStance)
{
	if (Stance == NewStance) return;
	Stance = NewStance;

	switch (Stance)
	{
	case Enum_Stance::Stand:
		TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
		CharacterMovement->bWantsToCrouch = false;
		break;
	case Enum_Stance::Crouch:
		TargetCapsuleHalfHeight = CrouchCapsuleHalfHeight;
		CharacterMovement->bWantsToCrouch = true;
		break;
	case Enum_Stance::Prone:
		TargetCapsuleHalfHeight = ProneCapsuleHalfHeight;
		CharacterMovement->bWantsToCrouch = false; // CMC crouch not used for prone
		break;
	}

	UpdateDynamicMovementSettings();
}

void ULocomotionComponent::SetProne(bool bWantsProne)
{
	if (!bIsValidCharacter) return;

	if (bWantsProne)
	{
		if (Stance != Enum_Stance::Crouch) return; // prone only from crouch
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

void ULocomotionComponent::SetMovementMode(Enum_MovementMode NewMovementMode)
{
	if (MovementMode == NewMovementMode) return;
	MovementMode = NewMovementMode;
	UpdateDynamicMovementSettings();
}

void ULocomotionComponent::SetCharacterState(Enum_CharacterState NewCharacterState)
{
	if (CharacterState == NewCharacterState) return;
	CharacterState = NewCharacterState;
}

// ── Speed Calculations ────────────────────────────────────────────────────────

Enum_Gait ULocomotionComponent::GetDesiredGait()
{
	if (CanSprint()) return Enum_Gait::Run;
	if (CharacterInputState.WantsToWalk) return Enum_Gait::Walk;
	return Enum_Gait::Jog;
}

float ULocomotionComponent::CalculateGroundFriction()
{
	if (Stance == Enum_Stance::Prone) return ProneGroundFriction;
	if (Gait == Enum_Gait::Jog || Gait == Enum_Gait::Walk) return WalkGroundFriction;
	return UKismetMathLibrary::MapRangeClamped(
		CharacterMovement->Velocity.Size2D(), 0.f, RunSpeed.X, WalkGroundFriction, RunGroundFriction);
}

float ULocomotionComponent::CalculateMaxAcceleration()
{
	if (Stance == Enum_Stance::Prone) return WalkAcceleration;
	if (Gait == Enum_Gait::Walk || Gait == Enum_Gait::Jog) return WalkAcceleration;
	return UKismetMathLibrary::MapRangeClamped(
		CharacterMovement->Velocity.Size2D(), WalkSpeedRelaxed.X * 1.35f, RunSpeed.X, WalkAcceleration, RunAcceleration);
}

float ULocomotionComponent::CalculateMaxBrakingDeceleration()
{
	return HasMovementInputVector() ? BrakingDeceleration : 2000.f;
}

float ULocomotionComponent::CalculateMaxSpeed()
{
	if (!IsValid(StrafeSpeedMapCurve)) return JogSpeed.X;

	float StrafeSpeedMap = CharacterMovement->bUseControllerDesiredRotation
		? StrafeSpeedMapCurve->GetFloatValue(FMath::Abs(UKismetAnimationLibrary::CalculateDirection(
			CharacterMovement->Velocity, Character->GetActorRotation())))
		: 0.f;

	FVector DesiredSpeed = JogSpeed;
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

	return StrafeSpeedMap < 1.f
		? UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.f, DesiredSpeed.X, DesiredSpeed.Y)
		: UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.f, DesiredSpeed.Y, DesiredSpeed.Z);
}

float ULocomotionComponent::CalculateMaxSpeedCrouched()
{
	if (!IsValid(StrafeSpeedMapCurve)) return CrouchSpeed.X;

	float StrafeSpeedMap = CharacterMovement->bOrientRotationToMovement
		? 0.f
		: StrafeSpeedMapCurve->GetFloatValue(FMath::Abs(UKismetAnimationLibrary::CalculateDirection(
			CharacterMovement->Velocity, Character->GetActorRotation())));

	return StrafeSpeedMap < 1.f
		? UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.f, CrouchSpeed.X, CrouchSpeed.Y)
		: UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.f, CrouchSpeed.Y, CrouchSpeed.Z);
}

float ULocomotionComponent::CalculateMaxSpeedProne()
{
	if (!IsValid(StrafeSpeedMapCurve)) return ProneSpeed.X;

	// Prone always strafes — always use the full directional map
	float StrafeSpeedMap = StrafeSpeedMapCurve->GetFloatValue(FMath::Abs(
		UKismetAnimationLibrary::CalculateDirection(CharacterMovement->Velocity, Character->GetActorRotation())));

	return StrafeSpeedMap < 1.f
		? UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.f, ProneSpeed.X, ProneSpeed.Y)
		: UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.f, ProneSpeed.Y, ProneSpeed.Z);
}

// ── Input ─────────────────────────────────────────────────────────────────────

void ULocomotionComponent::WantsToStrafe_Implementation()
{
	CharacterInputState.WantsToStrafe = !CharacterInputState.WantsToStrafe;
}

void ULocomotionComponent::WantsToRun_Implementation(bool bStarted)
{
	CharacterInputState.WantsToRun = bStarted;
}

void ULocomotionComponent::WantsToAim_Implementation(bool bStarted)
{
	CharacterInputState.WantsToAim = bStarted;
}

void ULocomotionComponent::WantsToWalk_Implementation(bool bStarted)
{
	CharacterInputState.WantsToWalk = bStarted;
}