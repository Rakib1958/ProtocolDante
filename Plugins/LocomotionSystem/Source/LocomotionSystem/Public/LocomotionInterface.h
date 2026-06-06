// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LocomotionStructures.h"
#include "LocomotionInterface.generated.h"
/**
 * 
 */
UINTERFACE(MinimalAPI, Blueprintable)
class ULocomotionInterface : public UInterface
{
	GENERATED_BODY()
};

class LOCOMOTIONSYSTEM_API ILocomotionInterface
{
	GENERATED_BODY()
public:
public:
	// A function that CAN be overridden in Blueprints, and MUST have a C++ fallback implementation.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface|Character Properties")
	void CallUpdateRotation();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface|Character Properties")
	void CallUpdateMovement();


	// only for the player pawns
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface|Player Properties")
	FStruct_GameplayCamera GetCharacterPropertiesForCamera();
};