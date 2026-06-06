#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class Enum_Gait : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run"),
	Sprint UMETA(DisplayName = "Sprint")
};

UENUM(BlueprintType)
enum class Enum_Stance : uint8
{
	Stand UMETA(DisplayName = "Stand"),
	Crouch UMETA(DisplayName = "Crouch")
};

UENUM(BlueprintType)
enum class Enum_MovementMode : uint8
{
	OnGround UMETA(DisplayName = "On Ground"),
	InAir UMETA(DisplayName = "In Air"),
	Ragdoll UMETA(DisplayName = "Ragdoll")
};

UENUM(BlueprintType)
enum class Enum_MovementState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Moving UMETA(DisplayName = "Moving")
};

UENUM(BlueprintType)
enum class Enum_CharacterState :uint8 {
	Relaxed UMETA(DisplayName = "Relaxed"),
	Stealth UMETA(DisplayName = "Stealth"),
	Combat UMETA(DisplayName = "Combat")
};

UENUM(BlueprintType)
enum class Enum_RotationMode : uint8
{
	VelocityDirection UMETA(DisplayName = "Velocity Direction"),
	LookingDirection UMETA(DisplayName = "Looking Direction")
};

UENUM(BlueprintType)
enum class Enum_CameraStyle : uint8
{
	Far UMETA(DisplayName = "Far"),
	Balanced UMETA(DisplayName = "Balanced"),
	Close UMETA(DisplayName = "Close"),
	Combat UMETA(DisplayName = "Combat")
};

UENUM(BlueprintType)
enum class Enum_CameraMode : uint8
{
	FreeCam UMETA(DisplayName = "FreeCam"),
	Strafe UMETA(DisplayName = "Strafe"),
	Aim UMETA(DisplayName = "Aim")
};

UENUM(BlueprintType)
enum class Enum_ViewMode : uint8
{
	ThirdPerson UMETA(DisplayName = "Third Person"),
	FirstPerson UMETA(DisplayName = "First Person")
};