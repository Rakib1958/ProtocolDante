// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LocomotionComponent.h"
#include "Perception/AISightTargetInterface.h" 
#include "BaseCharacter.generated.h"

UCLASS()
class CHARACTERDEMO_API ABaseCharacter : public ACharacter, public IAISightTargetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	ULocomotionComponent* LocomotionComponent = nullptr;

	// AI variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Variables")
	TArray<FName> SocketsToCheckForSight = { "head", "spine_03", "spine_04", "pelvis", "hand_l", "hand_r", "thigh_l", "thigh_r" };


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;



	// locomotion overrides
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	// perception overrides
		// override where the enemy can detect the character from, we will check multiple sockets to see if any of them are visible to the observer
	virtual bool CanBeSeenFrom(const FVector & ObserverLocation, FVector & OutSeenLocation, int32 & NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor * IgnoreActor = nullptr, const bool* bWasVisible = nullptr, int32 * UserData = nullptr) const;
	// override the ai perception component location and rotation to be from the character's head instead of the actor's origin
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
	UFUNCTION(BlueprintNativeEvent, Category = "AI Variables | Perception")
	void GetPerceptionLocationAndRotation(FVector& OutLocation, FRotator& OutRotation) const;

	UFUNCTION(BlueprintPure)
	FStruct_CharacterInputState GetCurrentInputState() const;
};
