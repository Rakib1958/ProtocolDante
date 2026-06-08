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


UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
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

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	ACharacter* Character;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UCharacterMovementComponent* CharacterMovement;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleComponent;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UAnimInstance* MainAnimInstance;

	bool IsValidCharacter = false;


public:


	void OnLandedEvent();


	UPROPERTY(EditAnywhere, Category = "Curves")
	UCurveFloat* StrafeSpeedMapCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States")
	Enum_Stance Stance = Enum_Stance::Stand;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States")
	Enum_Gait Gait = Enum_Gait::Run;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States")
	Enum_MovementMode MovementMode = Enum_MovementMode::OnGround;
	UPROPERTY(EditAnywhere, Category = "States")
	Enum_MovementState MovementState = Enum_MovementState::Idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	Enum_CharacterState CharacterState = Enum_CharacterState::Relaxed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	bool JustLanded = false;
	UPROPERTY(EditAnywhere, Category = "Speed")
	FVector CrouchSpeed = FVector(225.f, 200.f, 180.f);
	UPROPERTY(EditAnywhere, Category = "Speed")
	FVector SprintSpeed = FVector(585, 375.f, 375.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	FVector RunSpeed = FVector(375.f, 375.f, 375.f);
	UPROPERTY(EditAnywhere, Category = "Speed")
	FVector LandVelocity = FVector(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Speed")
	FVector WalkSpeedRelaxed = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, Category = "Speed")
	FVector WalkSpeedStealth = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, Category = "Speed")
	FVector WalkSpeedCombat = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, Category = "States")
	FStruct_CharacterInputState CharacterInputState;
	UPROPERTY(EditAnywhere, Category = "Friction")
	float SprintGroundFriction = 3.f;
	UPROPERTY(EditAnywhere, Category = "Friction")
	float WalkGroundFriction = 5.f;
	UPROPERTY(EditAnywhere, Category = "Acceleration")
	float SprintAcceleration = 300.f;
	UPROPERTY(EditAnywhere, Category = "Acceleration")
	float WalkAcceleration = 800.f;
	UPROPERTY(VisibleAnywhere, Category = "Acceleration")
	float BrakingFriction = 3.f;
	UPROPERTY(EditAnywhere, Category = "Acceleration")
	float BrakingDeceleration = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	UAnimMontage* RagdollGetUpBack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	UAnimMontage* RagdollGetUpFront;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FName PelvisBone = FName("pelvis");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FName SnapshotName = FName("RagdollPose");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	bool RagdollOnGround = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	bool RagdollFaceUp = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FVector LastRagdollVelocity = FVector(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement Analysis")
	bool IsSprinting() const;


	// ragdoll 
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ragdoll")
	void StartRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ragdoll")
	void StopRagdoll();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ragdoll")
	void UpdateRagdoll();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ragdoll")
	void SetActorLocationDuringRagdoll();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ragdoll")
	UAnimMontage* GetRagdollGetUpMontage();


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

	UFUNCTION(BlueprintCallable, Category = "Input")
	void WantsToSprint(bool IsHeld);
	UFUNCTION(BlueprintCallable, Category = "Input")
	void WantsToWalk(bool IsHeld);
	UFUNCTION(BlueprintCallable, Category = "Input")
	void WantsToAim(bool IsHeld);
	UFUNCTION(BlueprintCallable, Category = "Input")
	void WantsToStrafe();
	UFUNCTION(BlueprintCallable, Category = "Input")
	void WantsToCrouch();
};