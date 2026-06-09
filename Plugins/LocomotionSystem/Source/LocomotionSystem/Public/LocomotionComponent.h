// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LocomotionTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Curves/CurveFloat.h"
#include "Components/CapsuleComponent.h"
#include "LocomotionComponent.generated.h"


UCLASS( ClassGroup=(Custom), Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent) )
class LOCOMOTIONSYSTEM_API ULocomotionComponent : public UActorComponent
{
	GENERATED_BODY()
// default functions
public:	
	// Sets default values for this component's properties
	ULocomotionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


// custom methods and variables
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
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	bool bIsValidCharacter = false;

	void SetReferences();
	bool HasMovementInputVector();
	bool CanSprint() const;
	float CalculateMaxAcceleration();
	float CalculateMaxBrakingDeceleration();
	//float CalculateBrakingFriction();
	float CalculateGroundFriction();
	float CalculateMaxSpeed();
	float CalculateMaxSpeedCrouched();
	FGameplayTag GetDesiredGait() const;

protected:
	
	// curves
	UPROPERTY(EditAnywhere, Category = "Curves")
	UCurveFloat* StrafeSpeedMapCurve;

	

	// speed map
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector SprintSpeed = FVector(500.f, 500.f, 500.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector RunSpeed = FVector(375.f, 375.f, 375.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector CrouchSpeed = FVector(225.f, 200.f, 180.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector WalkSpeedRelaxed = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector WalkSpeedStealth = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector WalkSpeedCombat = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float WalkAcceleration = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float SprintAcceleration = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float BrakingDeceleration = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float WalkGroundFriction = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float SprintGroundFriction = 3.f;

	// ragdoll
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	UAnimMontage* RagdollGetUpBack = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	UAnimMontage* RagdollGetUpFront = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FName PelvisBone = "pelvis";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FName SnapshotName = "RagdollPose";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	bool bRagdollOnGround = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	bool bRagdollFaceUp = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FVector LastRagdollVelocity = FVector(0.f, 0.f, 0.f);

public:

	// public variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	FStruct_CharacterInputState CharacterInputState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	bool bJustLanded = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	float FlailRate = 0.f;

	// states
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	FGameplayTag Gait = LocomotionTags::Gait_Run;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	FGameplayTag Stance = LocomotionTags::Stance_Stand;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	FGameplayTag MovementMode = LocomotionTags::MovementMode_OnGround;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	FGameplayTag MovementState = LocomotionTags::MovementState_Idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	FGameplayTag CharacterState = LocomotionTags::CharacterState_Relaxed;

	// ragdoll overridable functions
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

	// update data
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "States")
	bool IsSprinting() const;
	UFUNCTION(BlueprintCallable, Category = "Update Data")
	void UpdateGait();
	UFUNCTION(BlueprintCallable, Category = "Update Data")
	void UpdateDynamicMovementSettings();


	// set state
	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetStance(FGameplayTag NewStance);
	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetMovementMode(FGameplayTag NewMovementMode);
	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetCharacterState(FGameplayTag NewCharacterState);
};
