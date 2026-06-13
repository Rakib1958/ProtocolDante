#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "GameplayCameraInterface.generated.h"

// ── Enumerations ─────────────────────────────────────────────────────────────
// for gameplay camera
UENUM(BlueprintType)
enum class Enum_CameraStyle : uint8
{
	Close    UMETA(DisplayName = "Close"),
	Balanced UMETA(DisplayName = "Balanced"),
	Far      UMETA(DisplayName = "Far"),
	Combat   UMETA(DisplayName = "Combat")
};

UENUM(BlueprintType)
enum class Enum_CameraMode : uint8
{
	Freecam UMETA(DisplayName = "Freecam"),
	Strafe  UMETA(DisplayName = "Strafe"),
	Aim     UMETA(DisplayName = "Aim")
};

// ── Rig Param Structs ─────────────────────────────────────────────────────────

// Full rig — used for BasePreset entries
USTRUCT(BlueprintType)
struct FStruct_CameraRigParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig")
	float TargetArmLength = 300.f;

	// lateral/vertical pivot shift (root of the spring arm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig")
	FVector PivotOffset = FVector(0.f, 0.f, 20.f);

	// camera end offset (shoulder positioning)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig")
	FVector SocketOffset = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig")
	float FieldOfView = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig")
	float LagSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig")
	float RotationLagSpeed = 10.f;

	// how fast this rig interpolates in — lower = smoother transition
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rig")
	float InterpSpeed = 5.f;
};

// Additive offset — used for StanceOffset and ShoulderOffset entries
USTRUCT(BlueprintType)
struct FStruct_CameraRigOffset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
	FVector PivotOffsetDelta = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
	FVector SocketOffsetDelta = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
	float ArmLengthDelta = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
	float FOVDelta = 0.f;

	// independent interp speed for this offset layer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
	float InterpSpeed = 6.f;
};

// ── Interface Structs ─────────────────────────────────────────────────────────
// for gameplay camera
USTRUCT(BlueprintType)
struct FStruct_GameplayCamera
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraStyle Style = Enum_CameraStyle::Balanced;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraMode Mode = Enum_CameraMode::Freecam;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCrouched = false;
};
// for springarm
USTRUCT(BlueprintType)
struct FStruct_SpringArmCamera
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag BasePreset;        // e.g. CameraSystem.Preset.LowProfile

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag StanceOffset;      // e.g. CameraSystem.Stance.Crouch

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ShoulderOffset;    // e.g. CameraSystem.Shoulder.Right
};

// ── Interface ─────────────────────────────────────────────────────────────────

UINTERFACE(MinimalAPI, Blueprintable)
class UGameplayCameraInterface : public UInterface
{
	GENERATED_BODY()
};

class CAMERASYSTEM_API IGameplayCameraInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Gameplay Camera")
	FStruct_GameplayCamera GetCharacterPropertiesForGameplayCamera();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SpringArm Camera")
	FStruct_SpringArmCamera GetCharacterPropertiesForSpringArmCamera();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SpringArm Camera")
	bool OverrideBaseCamera();
};