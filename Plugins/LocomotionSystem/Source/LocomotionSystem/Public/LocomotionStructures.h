// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionEnumerations.h"
#include "LocomotionStructures.generated.h"
/**
 * 
 */
USTRUCT(BlueprintType)
struct FStruct_CharacterInputState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToSprint = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToWalk = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToStrafe = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToAim = false;
};

USTRUCT(BlueprintType)
struct FStruct_GameplayCamera {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraStyle CameraStyle = Enum_CameraStyle::Balanced;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraMode CameraMode = Enum_CameraMode::FreeCam;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_Stance Stance = Enum_Stance::Stand;
};