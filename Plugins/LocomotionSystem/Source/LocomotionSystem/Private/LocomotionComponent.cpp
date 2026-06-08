#include "LocomotionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"

// Sets default values for this component's properties
ULocomotionComponent::ULocomotionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	SetReferences();
	// ...
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
}


// Called every frame
void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsValidCharacter)
	{
		switch (MovementMode)
		{
		case Enum_MovementMode::OnGround:
			if (Character->Implements<ULocomotionInterface>())
			{
				ILocomotionInterface::Execute_CallUpdateRotation(Character);
				ILocomotionInterface::Execute_CallUpdateMovement(Character);
			}
			break;
		case Enum_MovementMode::InAir:
			if (Character->Implements<ULocomotionInterface>())
			{
				ILocomotionInterface::Execute_CallUpdateRotation(Character);
				ILocomotionInterface::Execute_CallUpdateMovement(Character);
			}
			break;
		case Enum_MovementMode::Ragdoll:
			UpdateRagdoll();
			FlailRate = UKismetMathLibrary::MapRangeClamped(Mesh->GetPhysicsLinearVelocity().Size(), 0.f, 1000.f, 0.f, 1.f);
			break;
		default:
			break;
		}
	}
	// ...
}
void ULocomotionComponent::SetReferences()
{
	Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		CharacterMovement = Character->GetCharacterMovement();
		CapsuleComponent = Character->GetCapsuleComponent();
		Mesh = Character->GetMesh();
		MainAnimInstance = Mesh->GetAnimInstance();
		IsValidCharacter = true;
	}
}
void ULocomotionComponent::OnLandedEvent()
{
	LandVelocity = CharacterMovement->Velocity;
}

// conditions
bool ULocomotionComponent::HasMovementInputVector()
{
	return IsValidCharacter ? UKismetMathLibrary::NotEqual_VectorVector(Character->GetPendingMovementInputVector(), FVector::ZeroVector, 0.f) : false;
}
bool ULocomotionComponent::CanSprint()
{
	if (!IsValidCharacter)
	{
		return false;
	}
	return CharacterInputState.WantsToSprint 
		&& 
		(CharacterMovement->bOrientRotationToMovement 
			|| 
			(
				(UKismetMathLibrary::NormalizedDeltaRotator(
					Character->GetActorRotation(), 
					UKismetMathLibrary::MakeRotFromX(
						Character->IsLocallyControlled()
						? 
						Character->GetPendingMovementInputVector()
						: CharacterMovement->GetCurrentAcceleration())
				).Yaw)
				< 45.f
				)
			);
}
bool ULocomotionComponent::IsSprinting() const {
	return CharacterInputState.WantsToSprint && CharacterMovement->Velocity.SizeSquared() > pow(RunSpeed.X, 2);
}


// calculate
float ULocomotionComponent::CalculateMaxAcceleration()
{
	float returnvalue = 0.f;
	switch (Gait)
	{
	case Enum_Gait::Walk:
		returnvalue = WalkAcceleration;
		break;
	case Enum_Gait::Run:
		returnvalue = WalkAcceleration;
		break;
	case Enum_Gait::Sprint:
		returnvalue = UKismetMathLibrary::MapRangeClamped(CharacterMovement->Velocity.Size2D(), 250.f, SprintSpeed.X, WalkAcceleration, SprintAcceleration);
	}
	return returnvalue;
}
float ULocomotionComponent::CalculateMaxBrakingDeceleration()
{
	return HasMovementInputVector() ? BrakingDeceleration : 2000.f;
}
float ULocomotionComponent::CalculateBrakingFriction()
{
	return UKismetMathLibrary::EqualEqual_VectorVector(CharacterMovement->GetCurrentAcceleration(), FVector::ZeroVector, 0.f) ? 0.f : BrakingFriction;
}
float ULocomotionComponent::CalculateGroundFriction()
{
	float returnvalue = 0.f;
	switch (Gait)
	{
	case Enum_Gait::Walk:
		returnvalue = WalkGroundFriction;
		break;
	case Enum_Gait::Run:
		returnvalue = WalkGroundFriction;
		break;
	case Enum_Gait::Sprint:
		returnvalue = UKismetMathLibrary::MapRangeClamped(CharacterMovement->Velocity.Size2D(), 0.f, RunSpeed.X, WalkGroundFriction, SprintGroundFriction);
	}
	return returnvalue;
}
float ULocomotionComponent::CalculateMaxSpeed()
{
	if (!IsValid(StrafeSpeedMapCurve))
	{
		return RunSpeed.X;
	}
	float StrafeSpeedMap = CharacterMovement->bUseControllerDesiredRotation ? StrafeSpeedMapCurve->GetFloatValue(abs(UKismetAnimationLibrary::CalculateDirection(CharacterMovement->Velocity, Character->GetActorRotation()))) : 0.f;
	FVector DesiredSpeed = FVector::ZeroVector;
	switch (Gait)
	{
	case Enum_Gait::Walk:
		switch (CharacterState)
		{
		case Enum_CharacterState::Relaxed:
			DesiredSpeed = WalkSpeedRelaxed;
			break;
		case Enum_CharacterState::Stealth:
			DesiredSpeed = WalkSpeedStealth;
			break;
		case Enum_CharacterState::Combat:
			DesiredSpeed = WalkSpeedCombat;
			break;
		default:
			break;
		}
		break;
	case Enum_Gait::Run:
		DesiredSpeed = RunSpeed;
		break;
	case Enum_Gait::Sprint:
		DesiredSpeed = SprintSpeed;
		break;
	default:
		break;
	}
	return StrafeSpeedMap < 1.f ? UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.f, DesiredSpeed.X, DesiredSpeed.Y) : UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.f, DesiredSpeed.Y, DesiredSpeed.Z);
}
float ULocomotionComponent::CalculateMaxSpeedCrouched()
{
	if (!IsValid(StrafeSpeedMapCurve))
	{
		return CrouchSpeed.X;
	}
	float StrafeSpeedMap = CharacterMovement->bOrientRotationToMovement ? 0.f : StrafeSpeedMapCurve->GetFloatValue(FMath::Abs(UKismetAnimationLibrary::CalculateDirection(CharacterMovement->Velocity, Character->GetActorRotation())));
	return StrafeSpeedMap < 1.f ? UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.f, CrouchSpeed.X, CrouchSpeed.Y) : UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.f, CrouchSpeed.Y, CrouchSpeed.Z);
}
Enum_Gait ULocomotionComponent::GetDesiredGait()
{
	if (CanSprint())
	{
		return Enum_Gait::Sprint;
	}
	else if (CharacterInputState.WantsToWalk)
	{
		return Enum_Gait::Walk;
	}
	else
	{
		return Enum_Gait::Run;
	}
}


// input
void ULocomotionComponent::WantsToSprint(bool IsHeld)
{
	CharacterInputState.WantsToSprint = IsHeld;
}
void ULocomotionComponent::WantsToWalk(bool IsHeld)
{
	if (!CharacterInputState.WantsToSprint)
	{
		CharacterInputState.WantsToWalk = IsHeld;
	}
}
void ULocomotionComponent::WantsToAim(bool IsHeld)
{
	CharacterInputState.WantsToAim = IsHeld;
}
void ULocomotionComponent::WantsToStrafe()
{
	CharacterInputState.WantsToStrafe = !CharacterInputState.WantsToStrafe;
}
void ULocomotionComponent::WantsToCrouch()
{
	if (!CharacterMovement->IsFalling())
	{
		if (Character->IsCrouched())
		{
			Character->UnCrouch();
		}
		else {
			Character->Crouch();
		}
	}
}


// set states
void ULocomotionComponent::SetStance(Enum_Stance NewStance)
{
	if (Stance != NewStance)
	{
		Stance = NewStance;
	}
}
void ULocomotionComponent::SetMovementMode(Enum_MovementMode NewMovementMode)
{
	if (MovementMode != NewMovementMode)
	{
		MovementMode = NewMovementMode;
	}
}
void ULocomotionComponent::SetCharacterState(Enum_CharacterState NewCharacterState)
{
	if (CharacterState != NewCharacterState)
	{
		CharacterState = NewCharacterState;
	}
}
void ULocomotionComponent::UpdateGait()
{
	Enum_Gait NewGait = GetDesiredGait();
	if (NewGait != Gait)
	{
		Gait = NewGait;
	}
}
void ULocomotionComponent::UpdateDynamicMovementSettings()
{
	CharacterMovement->MaxAcceleration = CalculateMaxAcceleration();
	CharacterMovement->BrakingDecelerationWalking = CalculateMaxBrakingDeceleration();
	CharacterMovement->BrakingFriction = CalculateBrakingFriction();
	CharacterMovement->GroundFriction = CalculateGroundFriction();
	CharacterMovement->MaxWalkSpeed = CalculateMaxSpeed();
	CharacterMovement->MaxWalkSpeedCrouched = CalculateMaxSpeedCrouched();
}


// ragdoll
void ULocomotionComponent::StartRagdoll_Implementation()
{
	const FVector CharVelocity = CharacterMovement->Velocity;
	CharacterMovement->SetMovementMode(EMovementMode::MOVE_None);
	SetMovementMode(Enum_MovementMode::Ragdoll);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetAllBodiesBelowSimulatePhysics(PelvisBone, true, true);
	Mesh->SetPhysicsLinearVelocity(CharVelocity, false);
	MainAnimInstance->Montage_Stop(0.2f);
}
void ULocomotionComponent::StopRagdoll_Implementation()
{
	if (IsValid(MainAnimInstance))
	{
		MainAnimInstance->SavePoseSnapshot(SnapshotName);
	}
	if (RagdollOnGround)
	{
		CharacterMovement->SetMovementMode(EMovementMode::MOVE_Walking);
		MainAnimInstance->Montage_Play(GetRagdollGetUpMontage(), 1.0f);
	}
	else {
		CharacterMovement->SetMovementMode(EMovementMode::MOVE_Falling);
		CharacterMovement->Velocity = LastRagdollVelocity;
	}
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetAllBodiesSimulatePhysics(false);
}
void ULocomotionComponent::SetActorLocationDuringRagdoll_Implementation()
{
	FVector TargetLocation = Mesh->GetSocketLocation(PelvisBone);
	RagdollFaceUp = Mesh->GetSocketRotation(PelvisBone).Roll < 0.f;
	FRotator TargetRotation = FRotator(0.f, RagdollFaceUp ? Mesh->GetSocketRotation(PelvisBone).Yaw - 180.f : Mesh->GetSocketRotation(PelvisBone).Yaw, 0.f);
	FHitResult OutHit;
	GetWorld()->LineTraceSingleByChannel(OutHit, TargetLocation, TargetLocation - FVector(0.f, 0.f, CapsuleComponent->GetScaledCapsuleHalfHeight()), ECollisionChannel::ECC_Visibility);
	RagdollOnGround = OutHit.bBlockingHit;
	if (RagdollOnGround)
	{
		Character->SetActorLocationAndRotation(FVector(TargetLocation.X, TargetLocation.Y, TargetLocation.Z + 2.f + CapsuleComponent->GetScaledCapsuleHalfHeight() - FMath::Abs(OutHit.ImpactPoint.Z - OutHit.TraceStart.Z)), TargetRotation, false, nullptr, ETeleportType::None);
		/*FVector GroundedLocation = FVector(
			TargetLocation.X,
			TargetLocation.Y,
			TargetLocation.Z + 2.f + CapsuleComponent->GetScaledCapsuleHalfHeight() - FMath::Abs(OutHit.ImpactPoint.Z - OutHit.TraceStart.Z)
		);*/

		// CRITICAL FIX: Swap ETeleportType::None to ETeleportType::TeleportPhysics
		//Character->SetActorLocationAndRotation(GroundedLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else {
		Character->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::None);
		//Character->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}
void ULocomotionComponent::UpdateRagdoll_Implementation()
{
	LastRagdollVelocity = Mesh->GetPhysicsLinearVelocity("root");
	Mesh->SetAllMotorsAngularDriveParams(UKismetMathLibrary::MapRangeClamped(LastRagdollVelocity.Size(), 0.f, 1000.f, 0.f, 25000.f), 0.f, 0.f, false);
	Mesh->SetEnableGravity(LastRagdollVelocity.Z > -4000.f);
	SetActorLocationDuringRagdoll();
}
UAnimMontage* ULocomotionComponent::GetRagdollGetUpMontage_Implementation()
{
	return RagdollFaceUp ? RagdollGetUpBack : RagdollGetUpFront;
}