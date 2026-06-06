// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "LocomotionInterface.h"
#include "LocomotionStructures.h"
#include "LocomotionComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTIONSYSTEM_API ULocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULocomotionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "References")
	ACharacter* Character;
	UPROPERTY(VisibleAnywhere, Category = "References")
	UCharacterMovementComponent* CharacterMovement;
	UPROPERTY(VisibleAnywhere, Category = "References")
	UCapsuleComponent* CapsuleComponent;
	UPROPERTY(VisibleAnywhere, Category = "References")
	USkeletalMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere, Category = "References")
	UAnimInstance* MainAnimInstance;
	UPROPERTY(VisibleAnywhere, Category = "Curves")
	UCurveFloat* StrafeSpeedMapCurve;
	UPROPERTY(VisibleAnywhere, Category = "States")
	Enum_Stance Stance = Enum_Stance::Stand;
	UPROPERTY(VisibleAnywhere, Category = "States")
	Enum_Gait Gait = Enum_Gait::Run;
	UPROPERTY(VisibleAnywhere, Category = "States")
	Enum_MovementMode MovementMode = Enum_MovementMode::OnGround;
	UPROPERTY(VisibleAnywhere, Category = "States")
	Enum_MovementState MovementState = Enum_MovementState::Idle;
	UPROPERTY(VisibleAnywhere, Category = "States")
	Enum_CharacterState CharacterState = Enum_CharacterState::Relaxed;
	UPROPERTY(VisibleAnywhere, Category = "States")
	bool JustLanded = false;

	bool IsValidCharacter = false;


public:
	UPROPERTY(VisibleAnywhere, Category = "Speed")
	FVector CrouchSpeed = FVector(225.f, 200.f, 180.f);
	UPROPERTY(VisibleAnywhere, Category = "Speed")
	FVector SprintSpeed = FVector(585, 375.f, 375.f);
	UPROPERTY(VisibleAnywhere, Category = "Speed")
	FVector RunSpeed = FVector(375.f, 375.f, 375.f);
	UPROPERTY(VisibleAnywhere, Category = "Speed")
	FVector LandVelocity = FVector(0.f, 0.f, 0.f);
	UPROPERTY(VisibleAnywhere, Category = "Speed")
	FVector WalkSpeedRelaxed = FVector(165.f, 165.f, 165.f);
	UPROPERTY(VisibleAnywhere, Category = "Speed")
	FVector WalkSpeedStealth = FVector(165.f, 165.f, 165.f);
	UPROPERTY(VisibleAnywhere, Category = "Speed")
	FVector WalkSpeedCombat = FVector(165.f, 165.f, 165.f);
	UPROPERTY(VisibleAnywhere, Category = "States")
	FStruct_CharacterInputState CharacterInputState;
	UPROPERTY(VisibleAnywhere, Category = "Friction")
	float SprintGroundFriction = 3.f;
	UPROPERTY(VisibleAnywhere, Category = "Friction")
	float WalkGroundFriction = 5.f;
	UPROPERTY(VisibleAnywhere, Category = "Acceleration")
	float SprintAcceleration = 300.f;
	UPROPERTY(VisibleAnywhere, Category = "Acceleration")
	float WalkAcceleration = 800.f;
	UPROPERTY(VisibleAnywhere, Category = "Acceleration")
	float BrakingFriction = 3.f;
	UPROPERTY(VisibleAnywhere, Category = "Acceleration")
	float BrakingDeceleration = 500.f;
	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
	UAnimMontage* RagdollGetUpBack;
	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
	UAnimMontage* RagdollGetUpFront;
	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
	FName PelvisBone = FName("pelvis");
	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
	FName SnapshotName = FName("RagdollPose");
	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
	bool RagdollOnGround = false;
	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
	bool RagdollFaceUp = false;
	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
	FVector LastRagdollVelocity = FVector(0.f, 0.f, 0.f);
	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
	float FlailRate = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Set States")
	void SetStance(Enum_Stance NewStance);
	UFUNCTION(BlueprintCallable, Category = "Set States")
	void SetMovementMode(Enum_MovementMode NewMovementMode);
	UFUNCTION(BlueprintCallable, Category = "Set States")
	void SetCharacterState(Enum_CharacterState NewCharacterState);

	UFUNCTION(BlueprintCallable, Category = "Update Data")
	void UpdateGait();
	UFUNCTION(BlueprintCallable, Category = "Update Data")
	void UpdateDynamicMovementSettings();

	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	void StartRagdoll();
	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	void StopRagdoll();


private:
	void SetReferences();
	bool HasMovementInputVector();
	bool CanSprint();
	float CalculateMaxAcceleration();
	float CalculateMaxBrakingDeceleration();
	float CalculateBrakingFriction();
	float CalculateGroundFriction();
	float CalculateMaxSpeed();
	float CalculateMaxSpeedCrouched();
	Enum_Gait GetDesiredGait();
	UAnimMontage* GetRagdollGetUpMontage();
	void UpdateRagdoll();
	void SetActorLocationDuringRagdoll();

};