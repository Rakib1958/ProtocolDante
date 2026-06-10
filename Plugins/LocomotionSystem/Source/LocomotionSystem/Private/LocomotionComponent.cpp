// Fill out your copyright notice in the Description page of Project Settings.


#include "LocomotionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Engine/Engine.h"

// Sets default values for this component's properties
ULocomotionComponent::ULocomotionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// ...
}


// Called when the game starts
void ULocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	SetReferences();
	// ...
	
}

// tick
void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bIsValidCharacter) return;
	if (MovementMode == LocomotionTags::MovementMode_Ragdoll) return;
	OnUpdateMovement.Broadcast();
	OnUpdateRotation.Broadcast();
	
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
		if (IsValid(CharacterMovement) && IsValid(CapsuleComponent) && IsValid(Mesh) && IsValid(MainAnimInstance))
		{
			bIsValidCharacter = true;
		}
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
bool ULocomotionComponent::IsSprinting() const {
	return CharacterInputState.WantsToSprint && CharacterMovement->Velocity.SizeSquared() > pow(RunSpeed.X, 2);
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
void ULocomotionComponent::SetStance(FGameplayTag NewStance)
{
	if (Stance != NewStance)
		Stance = NewStance;
}
void ULocomotionComponent::SetMovementMode(FGameplayTag NewMovementMode)
{
	if (MovementMode != NewMovementMode)
		MovementMode = NewMovementMode;
}
void ULocomotionComponent::SetCharacterState(FGameplayTag NewCharacterState)
{
	if (CharacterState != NewCharacterState)
		CharacterState = NewCharacterState;
}


// ragdoll (just definitions)
void ULocomotionComponent::StartRagdoll_Implementation()
{
}
void ULocomotionComponent::StopRagdoll_Implementation()
{
}
void ULocomotionComponent::UpdateRagdoll_Implementation()
{
}
void ULocomotionComponent::SetActorLocationDuringRagdoll_Implementation()
{
}
UAnimMontage* ULocomotionComponent::GetRagdollGetUpMontage_Implementation() const
{
	return nullptr;
}

// calculations
float ULocomotionComponent::CalculateGroundFriction()
{
	if (Gait == LocomotionTags::Gait_Run || Gait == LocomotionTags::Gait_Walk) return WalkGroundFriction;
	return UKismetMathLibrary::MapRangeClamped(CharacterMovement->Velocity.Size2D(), 0.f, RunSpeed.X, WalkGroundFriction, SprintGroundFriction);
}
float ULocomotionComponent::CalculateMaxAcceleration()
{
	if (Gait == LocomotionTags::Gait_Walk || Gait == LocomotionTags::Gait_Run) return WalkAcceleration;
	return UKismetMathLibrary::MapRangeClamped(CharacterMovement->Velocity.Size2D(), WalkSpeedRelaxed.X * 1.35, SprintSpeed.X, WalkAcceleration, SprintAcceleration);
}
float ULocomotionComponent::CalculateMaxBrakingDeceleration()
{
	return HasMovementInputVector() ? BrakingDeceleration : 2000.f;
}
float ULocomotionComponent::CalculateMaxSpeed()
{
	if (!IsValid(StrafeSpeedMapCurve))
	{
		return RunSpeed.X;
	}
	float StrafeSpeedMap = CharacterMovement->bUseControllerDesiredRotation ? StrafeSpeedMapCurve->GetFloatValue(abs(UKismetAnimationLibrary::CalculateDirection(CharacterMovement->Velocity, Character->GetActorRotation()))) : 0.f;
	FVector DesiredSpeed = FVector::ZeroVector;
	if (Gait == LocomotionTags::Gait_Walk)
	{
		if (CharacterState == LocomotionTags::CharacterState_Relaxed) DesiredSpeed = WalkSpeedRelaxed;
		else if (CharacterState == LocomotionTags::CharacterState_Stealth) DesiredSpeed = WalkSpeedStealth;
		else {
			DesiredSpeed = WalkSpeedCombat;
		}
	}
	else if (Gait == LocomotionTags::Gait_Run)
	{
		DesiredSpeed = RunSpeed;
	}
	else {
		DesiredSpeed = SprintSpeed;
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
FGameplayTag ULocomotionComponent::GetDesiredGait() const
{
	if (CanSprint()) return LocomotionTags::Gait_Sprint;
	else if (CharacterInputState.WantsToWalk) return LocomotionTags::Gait_Walk;
	return LocomotionTags::Gait_Run;
}


// debug
void ULocomotionComponent::DebugPrint(FString text = TEXT("Hello"), float duration = 5.f, FColor color = FColor::Red)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,          // Key: Use -1 to create a new message line, or a positive integer to overwrite an existing one
			duration,        // Duration: How long the message stays on screen (in seconds)
			color,// Color: The text color
			text // The text macro
		);
	}
}


// input
void ULocomotionComponent::WantsToStrafe_Implementation()
{
	CharacterInputState.WantsToStrafe = !CharacterInputState.WantsToStrafe;
}
void ULocomotionComponent::WantsToSprint_Implementation(bool bIsHeld)
{
	CharacterInputState.WantsToSprint = bIsHeld;
}
void ULocomotionComponent::WantsToAim_Implementation(bool bIsHeld)
{
	CharacterInputState.WantsToAim = bIsHeld;
	CharacterInputState.WantsToWalk = bIsHeld;
}
void ULocomotionComponent::WantsToWalk_Implementation()
{
	CharacterInputState.WantsToWalk = !CharacterInputState.WantsToWalk;
}
void ULocomotionComponent::WantsToCrouch_Implementation()
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
void ULocomotionComponent::WantsToJump_Implementation(bool bIsHeld)
{
	if (bIsHeld)
	{
		if (Character->IsCrouched())
		{
			Character->UnCrouch();
		}
		else {
			Character->Jump();
		}
	}
	else {
		Character->StopJumping();
	}
}