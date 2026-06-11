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
}


// Called every frame
void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bIsValidCharacter) return;
	if (MovementMode == Enum_MovementMode::Ragdoll) return;
	OnUpdateMovement.Broadcast();
	OnUpdateRotation.Broadcast();
	// ...
}

// references
void ULocomotionComponent::SetReferences()
{
	Character = Cast<ACharacter>(GetOwner());
	if (IsValid(Character))
	{
		CharacterMovement = Character->GetCharacterMovement();
		CapsuleComponent = Character->GetCapsuleComponent();
		Mesh = Character->GetMesh();
		MainAnimInstance = Mesh->GetAnimInstance();
		if (IsValid(CharacterMovement) && IsValid(CapsuleComponent) && IsValid(Mesh) && IsValid(MainAnimInstance)) bIsValidCharacter = true;
	}
}

// conditions
bool ULocomotionComponent::HasMovementInputVector()
{
	return bIsValidCharacter ? UKismetMathLibrary::NotEqual_VectorVector(Character->GetPendingMovementInputVector(), FVector::ZeroVector, 0.f) : false;
}
bool ULocomotionComponent::CanSprint() const
{
	if (!bIsValidCharacter)
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
bool ULocomotionComponent::IsSprinting() {
	return CharacterInputState.WantsToSprint && CharacterMovement->Velocity.SizeSquared() > 0.f;
}

// update data
void ULocomotionComponent::UpdateGait()
{
	if (GetDesiredGait() != Gait) Gait = GetDesiredGait();
}
void ULocomotionComponent::UpdateDynamicMovementSettings()
{
	CharacterMovement->MaxAcceleration = CalculateMaxAcceleration();
	CharacterMovement->BrakingDecelerationWalking = CalculateMaxBrakingDeceleration();
	CharacterMovement->GroundFriction = CalculateGroundFriction();
	CharacterMovement->MaxWalkSpeed = CalculateMaxSpeed();
	CharacterMovement->MaxWalkSpeedCrouched = CalculateMaxSpeedCrouched();
}

// set state
void ULocomotionComponent::SetStance(Enum_Stance NewStance)
{
	if (Stance != NewStance)
		Stance = NewStance;
}
void ULocomotionComponent::SetMovementMode(Enum_MovementMode NewMovementMode)
{
	if (MovementMode != NewMovementMode)
		MovementMode = NewMovementMode;
}
void ULocomotionComponent::SetCharacterState(Enum_CharacterState NewCharacterState)
{
	if (CharacterState != NewCharacterState)
		CharacterState = NewCharacterState;
}



// ragdoll (just definitions)
//void ULocomotionComponent::StartRagdoll()
//{
//}
//void ULocomotionComponent::StopRagdoll()
//{
//}
//void ULocomotionComponent::UpdateRagdoll()
//{
//}
//void ULocomotionComponent::SetActorLocationDuringRagdoll()
//{
//}
//UAnimMontage* ULocomotionComponent::GetRagdollGetUpMontage() 
//{
//	return nullptr;
//}

// calculations
float ULocomotionComponent::CalculateGroundFriction()
{
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
	if (!IsValid(StrafeSpeedMapCurve))
	{
		return JogSpeed.X;
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
	case Enum_Gait::Jog:
		DesiredSpeed = JogSpeed;
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
	if (CanSprint()) return Enum_Gait::Run;
	else if (CharacterInputState.WantsToWalk) return Enum_Gait::Walk;
	return Enum_Gait::Jog;
}

// input
void ULocomotionComponent::WantsToStrafe_Implementation()
{
	CharacterInputState.WantsToStrafe = !CharacterInputState.WantsToStrafe;
}
void ULocomotionComponent::WantsToSprint_Implementation(bool bStarted)
{
	CharacterInputState.WantsToSprint = bStarted;
}
void ULocomotionComponent::WantsToAim_Implementation(bool bStarted)
{
	CharacterInputState.WantsToAim = bStarted;
}
void ULocomotionComponent::WantsToWalk_Implementation(bool bStarted)
{
	CharacterInputState.WantsToWalk = bStarted;
}

