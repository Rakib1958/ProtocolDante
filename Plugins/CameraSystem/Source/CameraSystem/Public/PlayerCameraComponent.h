// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PlayerCameraComponent.generated.h"

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

UCLASS( ClassGroup=(Custom), Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent) )
class CAMERASYSTEM_API UPlayerCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerCameraComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GameplayCamera")
	UCameraAsset* CameraRig = nullptr;
	

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GameplayCamera")
	void CreateGameplayCamera();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GameplayCamera")
	void InitCamera(bool bUseGameplayCamera);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Camera")
	void CreateCamera();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float TargetArmLength = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector SpringArmLocation = FVector(0.f, 0.f, 20.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator SpringArmRotation = FRotator(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float LagSpeed = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float RotationLagSpeed = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FieldOfView = 90.f;

private:
	void SetReference();

	// private variables
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	ACharacter* Character;
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "GameplayCamera")
	UGameplayCameraComponent* GameplayCameraRef = nullptr;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "GameplayCamera")
	USpringArmComponent* SpringArmRef = nullptr;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "GameplayCamera")
	UCameraComponent* CameraRef = nullptr;
	
	void InitializeCamera();
	void InitializeGameplayCamera();
	
		
};
