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

	// 1. Process capsule dimension updates smoothly across frame boundaries
	TickStanceTransition(DeltaTime);

	// 2. Predictive Ledge Checker: Triggers instant ragdoll falls if crawl limits are exceeded
	if (Stance == Enum_Stance::Prone)
	{
		CheckPredictiveProneLedgeFall();
	}

	// 3. Fire parallel design-layer blueprint delegates
	OnUpdateMovement.Broadcast();
	OnUpdateRotation.Broadcast();

	// ─── STABLE VELOCITY ROTATION RATE MAPPING ───
	if (Stance == Enum_Stance::Prone)
	{
		// Force standard velocity orientation rules cleanly
		CharacterMovement->bOrientRotationToMovement = true;
		CharacterMovement->bUseControllerDesiredRotation = false;
		Character->bUseControllerRotationYaw = false;

		// Default to our highly responsive tracking limit baseline
		float ActiveRotationRate = ProneMaxRotationRateYaw;

		// Apply dynamic damping curve ONLY if the player is actively moving and pushing a stick input
		if (CharacterMovement->Velocity.Size2D() > 15.f && HasMovementInputVector())
		{
			FVector InputDirection = Character->GetPendingMovementInputVector().GetSafeNormal2D();
			FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(InputDirection);

			// Measure the explicit absolute angular deviation between his heading and his intent
			float DeltaYaw = FMath::Abs(UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, Character->GetActorRotation()).Yaw);

			if (DeltaYaw > 0.1f)
			{
				// Smoothly scale the turning speed down as the steering angle gets steeper (0° to 90°)
				ActiveRotationRate = UKismetMathLibrary::MapRangeClamped(
					DeltaYaw,
					0.f, 90.f,
					ProneMaxRotationRateYaw,
					ProneMinRotationRateYaw
				);
			}
		}

		// Ship the calculated smooth dampening value straight to the physics layer
		CharacterMovement->RotationRate = FRotator(0.f, ActiveRotationRate, 0.f);
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

			CapsuleComponent->SetCapsuleHalfHeight(StandCapsuleHalfHeight);
			CharacterMovement->CrouchedHalfHeight = CrouchCapsuleHalfHeight;
			TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
		}
	}
}

// ─── HIGH RESOLUTION EARLY FALL RAYCAST TRACKER ───
void ULocomotionComponent::CheckPredictiveProneLedgeFall()
{
	// Only calculate early drop-offs if Dante is actively crawling forward
	if (CharacterMovement->Velocity.Size2D() < 15.f) return;

	FVector CoreLocation = Character->GetActorLocation();
	FVector MoveDirection = CharacterMovement->Velocity.GetSafeNormal2D();

	// Project trace origin 45 units forward from capsule center to match his physical head line position
	FVector TraceStart = CoreLocation + (MoveDirection * 45.f);
	FVector TraceEnd = TraceStart - FVector(0.f, 0.f, ProneCapsuleHalfHeight + 35.f);

	FHitResult LedgeHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	// Perform visibility raycast channel check down past floor support boundaries
	bool bGroundExistsAhead = Character->GetWorld()->LineTraceSingleByChannel(
		LedgeHit, TraceStart, TraceEnd, ECC_Visibility, Params
	);

	// If head/torso extends past support boundaries over open air, execute ragdoll collapse instantly
	if (!bGroundExistsAhead)
	{
		SetMovementMode(Enum_MovementMode::Ragdoll);
	}
}

// ─── UNIVERSAL STANCE INTERPOLATION CORE ───
void ULocomotionComponent::TickStanceTransition(float DeltaTime)
{
	float Current = CapsuleComponent->GetUnscaledCapsuleHalfHeight();

	if (FMath::IsNearlyEqual(Current, TargetCapsuleHalfHeight, 0.01f))
	{
		CapsuleComponent->SetCapsuleHalfHeight(TargetCapsuleHalfHeight, true);

		FVector MeshRelLoc = Mesh->GetRelativeLocation();
		MeshRelLoc.Z = -TargetCapsuleHalfHeight;
		Mesh->SetRelativeLocation(MeshRelLoc);
		return;
	}

	float NewHeight = FMath::FInterpTo(Current, TargetCapsuleHalfHeight, DeltaTime, StanceTransitionSpeed);
	float HeightDelta = Current - NewHeight;

	FVector Loc = Character->GetActorLocation();
	Loc.Z -= HeightDelta;
	Character->SetActorLocation(Loc, false); // bSweep = false protects animation solvers from twitching

	CapsuleComponent->SetCapsuleHalfHeight(NewHeight, true);

	FVector MeshRelLoc = Mesh->GetRelativeLocation();
	MeshRelLoc.Z = -NewHeight;
	Mesh->SetRelativeLocation(MeshRelLoc);
}

// ─── DATA MATRIX QUERIES ───
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

	float TraceDistance = CrouchCapsuleHalfHeight - ProneCapsuleHalfHeight;
	FVector Start = Character->GetActorLocation();
	FVector End = Start + FVector(0.f, 0.f, TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	return !Character->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}

// ─── VALUE MATRIX HANDLERS ───
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

// ─── STATE INFRASTRUCTURE ROUTERS ───
void ULocomotionComponent::SetStance(Enum_Stance NewStance)
{
	if (Stance == NewStance) return;
	Stance = NewStance;

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
	if (MovementMode == NewMovementMode) return;

	// Intercept core engine fall modes: If falling or early-raycast triggered while prone, route to Ragdoll instantly
	if ((NewMovementMode == Enum_MovementMode::InAir || NewMovementMode == Enum_MovementMode::Ragdoll) && Stance == Enum_Stance::Prone)
	{
		MovementMode = Enum_MovementMode::Ragdoll;
		UpdateDynamicMovementSettings();

		StartRagdoll(); // Invokes Blueprint system to decouple skeletal bounds into full simulation
		return;
	}

	MovementMode = NewMovementMode;
	UpdateDynamicMovementSettings();
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
	}
	else
	{
		if (!CanStandFromProne()) return;
		SetStance(Enum_Stance::Crouch);
	}
}

// ─── MATHEMATICAL SPEED MAPS ───
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

// ─── NATIVE HANDLING LINKAGE PASSTHROUGHS ───
void ULocomotionComponent::WantsToStrafe_Implementation() { CharacterInputState.WantsToStrafe = !CharacterInputState.WantsToStrafe; }
void ULocomotionComponent::WantsToRun_Implementation(bool bStarted) { CharacterInputState.WantsToRun = bStarted; }
void ULocomotionComponent::WantsToAim_Implementation(bool bStarted) { CharacterInputState.WantsToAim = bStarted; }
void ULocomotionComponent::WantsToWalk_Implementation(bool bStarted) { CharacterInputState.WantsToWalk = bStarted; }