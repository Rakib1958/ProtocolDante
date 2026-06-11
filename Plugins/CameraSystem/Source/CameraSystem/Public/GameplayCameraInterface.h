#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayCameraInterface.generated.h"

// enumerations
UENUM(BlueprintType)
enum class Enum_CameraStyle : uint8 {
	Close UMETA(DisplayName = "Close"),
	Balanced UMETA(DisplayName = "Balanced"),
	Far UMETA(DisplayName = "Far"),
	Combat UMETA(DisplayName = "Combat")
};
UENUM(BlueprintType)
enum class Enum_CameraMode : uint8
{
	Freecam UMETA(DisplayName = "Freecam"),
	Strafe UMETA(DisplayName = "Strafe"),
	Aim UMETA(DisplayName = "Aim")
};
UENUM(BlueprintType)
enum class Enum_CameraBasePreset : uint8 {
	LowProfile UMETA(DisplayName = "Low Profile"),
	HighProfile UMETA(DisplayName = "High Profile"),
	Aiming UMETA(DisplayName = "Aiming"),
	Combat UMETA(DisplayName = "Combat")
};
UENUM(BlueprintType)
enum class Enum_CameraStanceOffset : uint8 {
	Stand UMETA(DisplayName = "Stand"),
	Crouch UMETA(DisplayName = "Crouch")
};
UENUM(BlueprintType)
enum class Enum_CameraShoulderOffset : uint8 {
	Right UMETA(DisplayName = "Right"),
	Left UMETA(DisplayName = "Left")
};


// structures
USTRUCT(BlueprintType)
struct FStruct_GameplayCamera {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraStyle Style = Enum_CameraStyle::Balanced;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraMode Mode = Enum_CameraMode::Freecam;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCrouched = false;
};
USTRUCT(BlueprintType)
struct FStruct_SpringArmCamera
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraBasePreset BasePreset = Enum_CameraBasePreset::LowProfile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraStanceOffset StanceOffset = Enum_CameraStanceOffset::Stand;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	Enum_CameraShoulderOffset ShoulderOffset = Enum_CameraShoulderOffset::Right;
};

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
	void OverrideBaseCamera(bool bOverride);
};
