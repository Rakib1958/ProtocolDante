#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameplayCameraInterface.h"
#include "PlayerCameraComponent.generated.h"


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

protected:
	// default camera params
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	float TargetArmLength = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	FVector SpringArmLocation = FVector(0.f, 0.f, 20.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	FRotator SpringArmRotation = FRotator(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	float LagSpeed = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	float RotationLagSpeed = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	float FieldOfView = 90.f;
	
public:
	// this must be overriden in blueprint to use gameplay camera
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GameplayCamera")
	void SetCameraReference();
	UFUNCTION(BlueprintCallable, Category = "Initialize")
	void InitCamera(bool bUseGameplayCamera);


private:
	
	// these are invisible in blueprint

	// private variables
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	ACharacter* Character;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "GameplayCamera")
	UGameplayCameraComponent* GameplayCameraRef = nullptr;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Camera")
	USpringArmComponent* SpringArmRef = nullptr;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Camera")
	UCameraComponent* CameraRef = nullptr;

	// private methods
	void SetReference();
	void CreateCamera();
	void InitializeCamera();
	void CreateGameplayCamera();
	void InitializeGameplayCamera();

// custom camera tick
public:

	// current desired camera properties
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera|Properties")
	FStruct_SpringArmCamera CurrentDesiredCameraProperties;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera|Override")
	bool bIsOverrideActive = false;

};
