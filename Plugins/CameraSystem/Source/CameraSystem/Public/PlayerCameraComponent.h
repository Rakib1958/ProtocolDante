#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameplayCameraInterface.h"
#include "PlayerCameraComponent.generated.h"

UCLASS(ClassGroup = (Custom), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class CAMERASYSTEM_API UPlayerCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerCameraComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ── Initialization ────────────────────────────────────────────────────────

	// Override in Blueprint to assign GameplayCamera asset reference
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GameplayCamera")
	void SetCameraReference();

	UFUNCTION(BlueprintCallable, Category = "Initialize")
	void InitCamera(bool bUseGameplayCamera);

	// ── Preset Maps — fill in Blueprint Details panel ─────────────────────────

	// Full rig per base preset (LowProfile, HighProfile, Aiming, Combat)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets")
	TMap<Enum_CameraBasePreset, FStruct_CameraRigParams> BasePresetMap;

	// Additive stance offset (Stand = zero offset, Crouch = drops pivot etc)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets")
	TMap<Enum_CameraStanceOffset, FStruct_CameraRigOffset> StanceOffsetMap;

	// Additive shoulder offset (Left/Right socket offset only)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets")
	TMap<Enum_CameraShoulderOffset, FStruct_CameraRigOffset> ShoulderOffsetMap;

	// ── Runtime State ─────────────────────────────────────────────────────────

	// Written by Blueprint or interface each tick — drives evaluation
	UPROPERTY(BlueprintReadWrite, Category = "Camera|State")
	FStruct_SpringArmCamera DesiredCameraState;

	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	bool bIsOverrideActive = false;

	// ── Capsule Z Smoothing ───────────────────────────────────────────────────

	// How fast the spring arm pivot chases capsule height changes (crouch snap fix)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
	float CrouchZInterpSpeed = 8.f;

	// ── Default Values (fallback / initial spring arm setup) ─────────────────

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	float TargetArmLength = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	FVector SpringArmLocation = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	FRotator SpringArmRotation = FRotator(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	float LagSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	float RotationLagSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Default Values")
	float FieldOfView = 90.f;

private:
	// ── Component References ──────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	ACharacter* Character = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "GameplayCamera")
	UGameplayCameraComponent* GameplayCameraRef = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Camera")
	USpringArmComponent* SpringArmRef = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Camera")
	UCameraComponent* CameraRef = nullptr;

	// ── Interpolated State (internal) ─────────────────────────────────────────

	// Current interpolated values for each layer
	FStruct_CameraRigParams CurrentBase;
	FStruct_CameraRigOffset CurrentStanceOffset;
	FStruct_CameraRigOffset CurrentShoulderOffset;

	// Override rig target and its interp speed
	FStruct_CameraRigParams OverrideRig;
	float OverrideInterpSpeed = 5.f;
	float RestoreInterpSpeed = 3.f;

	// Smoothed capsule Z — avoids snap on crouch
	float SmoothedPivotZ = 0.f;
	bool  bPivotZInitialized = false;

	// Collision correction values
	float CollisionArmCorrection = 0.f;
	float CollisionOffsetCorrection = 0.f;

	// ── Private Methods ───────────────────────────────────────────────────────

	void SetReference();
	void CreateCamera();
	void InitializeCamera();
	void CreateGameplayCamera();
	void InitializeGameplayCamera();

	// Tick sub-steps
	void EvaluateAndInterpBase(float DeltaTime);
	void EvaluateAndInterpStanceOffset(float DeltaTime);
	void EvaluateAndInterpShoulderOffset(float DeltaTime);
	void UpdateCapsuleZSmoothing(float DeltaTime);
	void UpdateCollision(float DeltaTime);
	void ApplyToSpringArm();
};