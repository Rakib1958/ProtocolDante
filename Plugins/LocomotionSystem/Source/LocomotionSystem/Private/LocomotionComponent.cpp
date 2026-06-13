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
		HandleProneExtremityCollisions(DeltaTime); // ◄ Added manual sweep pass
	}

	// 3. Fire parallel design-layer blueprint delegates
	OnUpdateMovement.Broadcast();
	OnUpdateRotation.Broadcast();

	// ─── STABLE VELOCITY ROTATION RATE MAPPING ───
	if (Stance == Enum_Stance::Prone)
	{
		SetRotationWhileProning();
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
			// only if all references are true, we can proceed to evaluating this component
			CapsuleComponent->SetCapsuleHalfHeight(StandCapsuleHalfHeight);
			CharacterMovement->SetCrouchedHalfHeight(CrouchCapsuleHalfHeight);
			TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
		}
	}
}

// Prone system
void ULocomotionComponent::CheckPredictiveProneLedgeFall()
{
	// Only calculate early drop-offs if Dante is actively crawling forward
	if (CharacterMovement->Velocity.Size2D() < 15.f) return;

	FVector CoreLocation = Character->GetActorLocation();
	FVector MoveDirection = CharacterMovement->Velocity.GetSafeNormal2D();

	// Project trace origin 45 units forward from capsule center to match his physical head line position
	FVector TraceStart = CoreLocation + (MoveDirection * 40.f);
	FVector TraceEnd = TraceStart - FVector(0.f, 0.f, CapsuleComponent->GetUnscaledCapsuleHalfHeight() + 35.f);

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
		StartRagdoll();
	}
}
void ULocomotionComponent::SetRotationWhileProning()
{
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
				0.f, 180.f,
				ProneMaxRotationRateYaw,
				ProneMinRotationRateYaw
			);
		}
	}
	// Ship the calculated smooth dampening value straight to the physics layer
	CharacterMovement->RotationRate = FRotator(0.f, ActiveRotationRate, 0.f);
}
void ULocomotionComponent::HandleProneExtremityCollisions(float DeltaTime)
{
	if (!bIsValidCharacter || !IsValid(CharacterMovement) || !IsValid(CapsuleComponent)) return;
	// we need to handle the prone state collision manually as the capsule is no longer doing that for us
	FVector CapsuleCenter = CapsuleComponent->GetComponentLocation();
	FVector ForwardVector = Character->GetActorForwardVector();
	// so we trace twice once from upperbody and one from lower body to detect whether there's something blocking the way
	FCollisionShape BodySphere = FCollisionShape::MakeSphere(ProneExtremityRadius);
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Character);
	FHitResult HitInfo;

	// ─── PASS 1: HEAD / UPPER BODY PREDICTIVE SWEEP ───
	FVector HeadTraceEnd = CapsuleCenter + (ForwardVector * ProneHeadCheckLength);
	bool bHeadHit = Character->GetWorld()->SweepSingleByChannel(
		HitInfo,
		CapsuleCenter, HeadTraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		BodySphere, TraceParams
	);
	if (bHeadHit)
	{
		FVector CurrentVelocity = CharacterMovement->Velocity;

		// Verify if the player's momentum is actively pushing INTO the wall surface
		if (FVector::DotProduct(CurrentVelocity, HitInfo.Normal) < 0.f)
		{
			// Project the velocity onto the wall's plane. Handles wall sliding cleanly.
			CharacterMovement->Velocity = FVector::VectorPlaneProject(CurrentVelocity, HitInfo.Normal);
			// Clear out any structural forces accumulated during this physics tick sub-step
			CharacterMovement->ClearAccumulatedForces();
			// Natively consume the pending controller input vector for this frame to kill driving power
			Character->ConsumeMovementInputVector();
		}
		// Refresh our tracking center coordinate loop for the sequential leg check pass
		CapsuleCenter = CapsuleComponent->GetComponentLocation();
	}
	// ─── PASS 2: LEGS / LOWER BODY PREDICTIVE SWEEP ───
	FVector LegTraceEnd = CapsuleCenter - (ForwardVector * ProneLegCheckLength);
	bool bLegsHit = Character->GetWorld()->SweepSingleByChannel(
		HitInfo,
		CapsuleCenter, LegTraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		BodySphere, TraceParams
	);
	if (bLegsHit)
	{
		FVector CurrentVelocity = CharacterMovement->Velocity;
		// Verify if the player is actively reversing or backing into an obstacle
		if (FVector::DotProduct(CurrentVelocity, HitInfo.Normal) < 0.f)
		{
			CharacterMovement->Velocity = FVector::VectorPlaneProject(CurrentVelocity, HitInfo.Normal);
			CharacterMovement->ClearAccumulatedForces();
			Character->ConsumeMovementInputVector();
		}
	}
} // very very important
void ULocomotionComponent::TickStanceTransition(float DeltaTime)
{
	// smoothly handles capsule shrinking which helps to nullify jitters while stand->crouch, or stand->prone etc
	float CurrentHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	// snap to the target if the values are really close
	if (FMath::IsNearlyEqual(CurrentHalfHeight, TargetCapsuleHalfHeight, 0.01f))
	{
		CapsuleComponent->SetCapsuleHalfHeight(TargetCapsuleHalfHeight, true);

		FVector MeshRelativeLocation = Mesh->GetRelativeLocation();
		MeshRelativeLocation.Z = -TargetCapsuleHalfHeight;
		Mesh->SetRelativeLocation(MeshRelativeLocation);
		return;
	}
	// if the difference is very high, smoothly interpolate close to that value
	float NewHeight = FMath::FInterpTo(CurrentHalfHeight, TargetCapsuleHalfHeight, DeltaTime, StanceTransitionSpeed);
	float HeightDelta = CurrentHalfHeight - NewHeight;
	// if we dont move the actor and mesh location, when the capsule shrinks such as stand to prone, the character will be midair a split second, and it seems like he is falling on the ground and then proning
	FVector CurrentLocation = Character->GetActorLocation();
	CurrentLocation.Z -= HeightDelta;
	Character->SetActorLocation(CurrentLocation, false); 
	// then change the capsule half height to catch up to the player, otherwise he might fall through the ground
	CapsuleComponent->SetCapsuleHalfHeight(NewHeight, true);
	FVector MeshRelativeLocation = Mesh->GetRelativeLocation();
	MeshRelativeLocation.Z = -NewHeight;
	Mesh->SetRelativeLocation(MeshRelativeLocation);
}

// simple debug system
void ULocomotionComponent::DebugPrint(FString text, FColor color, float duration)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, duration, color, text);
	}
}

// necessary conditions
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
				UKismetMathLibrary::MakeRotFromX(Character->IsLocallyControlled() ? Character->GetPendingMovementInputVector() : CharacterMovement->GetCurrentAcceleration())).Yaw) < 55.f));
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

// methods to update movement settings
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

// Switch between different states
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
	{ // we only prone to crouch when there is enough space above the player
		if (!CanStandFromProne()) return;
		SetStance(Enum_Stance::Crouch);
	}
}

// Calculate current movement settings
float ULocomotionComponent::CalculateMaxSpeedProned()
{
	// if no map is assigned return default prone forward speed
	if (!IsValid(StrafeSpeedMapCurve)) return ProneSpeed.X;

	float DesiredSpeed = StrafeSpeedMapCurve->GetFloatValue(
		FMath::Abs(
			UKismetAnimationLibrary::CalculateDirection(
				CharacterMovement->Velocity, Character->GetActorRotation())));

	return (DesiredSpeed < 1.f) ?
		UKismetMathLibrary::MapRangeClamped( // forward direction speed (0 to 180 degree)
			DesiredSpeed, 0.f, 1.f, ProneSpeed.X, ProneSpeed.Y) : 
		UKismetMathLibrary::MapRangeClamped( // backward direction speed(180 to 360 degree)
			DesiredSpeed, 1.f, 2.f, ProneSpeed.Y, ProneSpeed.Z);
}
float ULocomotionComponent::CalculateGroundFriction()
{
	if (Stance == Enum_Stance::Prone) return ProneGroundFriction;
	if (Gait == Enum_Gait::Jog || Gait == Enum_Gait::Walk) return WalkGroundFriction;
	return UKismetMathLibrary::MapRangeClamped(
		CharacterMovement->Velocity.Size2D(), 
		0.f, 
		RunSpeed.X, 
		WalkGroundFriction, 
		RunGroundFriction);
}
float ULocomotionComponent::CalculateMaxAcceleration()
{
	if (Gait == Enum_Gait::Walk || Gait == Enum_Gait::Jog) return WalkAcceleration;
	return UKismetMathLibrary::MapRangeClamped(
		CharacterMovement->Velocity.Size2D(), 
		WalkSpeedRelaxed.X * 1.35, 
		RunSpeed.X, 
		WalkAcceleration, 
		RunAcceleration);
}
float ULocomotionComponent::CalculateMaxBrakingDeceleration()
{
	return HasMovementInputVector() ? BrakingDeceleration : 2000.f;
}
float ULocomotionComponent::CalculateMaxSpeed()
{
	// if no map is assigned in editor, return the default run speed
	if (!IsValid(StrafeSpeedMapCurve)) return JogSpeed.X;

	float StrafeSpeedMap = (CharacterMovement->bUseControllerDesiredRotation) ? 
		StrafeSpeedMapCurve->GetFloatValue(FMath::Abs(
			UKismetAnimationLibrary::CalculateDirection(
				CharacterMovement->Velocity, Character->GetActorRotation()))) : 
		0.f;
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

	return (StrafeSpeedMap < 1.f) ? 
		UKismetMathLibrary::MapRangeClamped( // forward direction speed (0 to 180 degree)
			StrafeSpeedMap, 0.f, 1.f, DesiredSpeed.X, DesiredSpeed.Y) : 
		UKismetMathLibrary::MapRangeClamped( // backward direction speed(180 to 360 degree)
			StrafeSpeedMap, 1.f, 2.f, DesiredSpeed.Y, DesiredSpeed.Z);
}
float ULocomotionComponent::CalculateMaxSpeedCrouched()
{
	// return default crouch speed if no map is assigned in editor
	if (!IsValid(StrafeSpeedMapCurve)) return CrouchSpeed.X;
	// if strafing then get the 4 directional speeds extracted from the strafe map and assigned speeds
	float StrafeSpeedMap = (CharacterMovement->bOrientRotationToMovement) ? 0.f
		: StrafeSpeedMapCurve->GetFloatValue(
			FMath::Abs(
				UKismetAnimationLibrary::CalculateDirection(
					CharacterMovement->Velocity, 
					Character->GetActorRotation())));
	return (StrafeSpeedMap < 1.f) ? 
		UKismetMathLibrary::MapRangeClamped( // forward direction speed (0 to 180 degree)
			StrafeSpeedMap, 0.f, 1.f, CrouchSpeed.X, CrouchSpeed.Y) : 
		UKismetMathLibrary::MapRangeClamped( // backward direction speed(180 to 360 degree)
			StrafeSpeedMap, 1.f, 2.f, CrouchSpeed.Y, CrouchSpeed.Z);
}
Enum_Gait ULocomotionComponent::GetDesiredGait()
{
	if (CanSprint()) return Enum_Gait::Run;
	if (CharacterInputState.WantsToWalk) return Enum_Gait::Walk;
	return Enum_Gait::Jog;
}

// Handle Player Inputs
void ULocomotionComponent::WantsToStrafe_Implementation() { CharacterInputState.WantsToStrafe = !CharacterInputState.WantsToStrafe; }
void ULocomotionComponent::WantsToRun_Implementation(bool bStarted) { CharacterInputState.WantsToRun = bStarted; }
void ULocomotionComponent::WantsToAim_Implementation(bool bStarted) { CharacterInputState.WantsToAim = bStarted; }
void ULocomotionComponent::WantsToWalk_Implementation(bool bStarted) { CharacterInputState.WantsToWalk = bStarted; }