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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets")
	TMap<FGameplayTag, FStruct_CameraRigParams> BasePresetMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets")
	TMap<FGameplayTag, FStruct_CameraRigOffset> StanceOffsetMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets")
	TMap<FGameplayTag, FStruct_CameraRigOffset> ShoulderOffsetMap;

	// ── Runtime State ─────────────────────────────────────────────────────────

	// Written by Blueprint or interface each tick — drives evaluation
	UPROPERTY(BlueprintReadWrite, Category = "Camera|State")
	FStruct_SpringArmCamera DesiredCameraState;

	// True while the notify override layer is blending in or holding
	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	bool bIsOverrideActive = false;

	// ── Anim-Notify Camera Override ───────────────────────────────────────────

	// Called from AnimNotifyState::NotifyBegin (via Blueprint).
	// DefaultParams   — where to restore to when the notify ends.
	// TargetParams    — where to interpolate toward right now.
	UFUNCTION(BlueprintCallable, Category = "Camera|Notify Override")
	void BeginCameraOverride(FStruct_CameraNotifyParams DefaultParams,
		FStruct_CameraNotifyParams TargetParams);

	// Called from AnimNotifyState::NotifyEnd (via Blueprint).
	// Kicks off the interpolation back toward the DefaultParams that were
	// supplied at BeginCameraOverride; clears the override once settled.
	UFUNCTION(BlueprintCallable, Category = "Camera|Notify Override")
	void EndCameraOverride();

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

	// ── FIRST PERSON CONFIGURATIONS ──────────────────────────────────────────

	/** Forward structural offset from the head bone origin to clear eye/nose mesh geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | First Person")
	FVector FPSMeshOffset = FVector(15.f, 0.f, 5.f);

	/** Maximum vertical view ceiling when standing or crouching in FPS (Default: 70 degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | First Person")
	float FPSNormalPitchLimit = 70.f;

	/** Tighter vertical view ceiling when flat on stomach to prevent looking through the chest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | First Person")
	float FPSPronePitchLimit = 35.f;

	/** Camera rotation smoothing delay when prone. Lower values make the view feel heavily sluggish. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | First Person")
	float FPSProneRotationLag = 3.0f;

private:
	// ── Component References ──────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	ACharacter* Character = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UCharacterMovementComponent* CharacterMovement = nullptr;

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

	FGameplayTag LastStanceOffset = FGameplayTag::RequestGameplayTag(TEXT("CameraSystem.Stance.Stand"));
	bool bIsStanceTransitioning = false;

	// Smoothed capsule Z — avoids snap on crouch
	float SmoothedPivotZ = 0.f;
	bool  bPivotZInitialized = false;

	float FallFOVReduction = 0.f;
	float FallArmReduction = 0.f;

	// ── Notify Override State (internal) ──────────────────────────────────────

	// The two bookend param sets supplied by the AnimNotifyState
	FStruct_CameraNotifyParams NotifyDefaultParams;   // restore target
	FStruct_CameraNotifyParams NotifyTargetParams;    // enter target

	// Live interpolated override values — applied additively in ApplyToSpringArm
	FStruct_CameraNotifyParams CurrentNotifyOverride;

	// True  → interpolating toward NotifyTargetParams  (NotifyBegin)
	// False → interpolating toward NotifyDefaultParams (NotifyEnd)
	bool bNotifyBlendingIn = false;

	// Once EndCameraOverride is called we blend back; when close enough we
	// clear bIsOverrideActive entirely.
	bool bNotifyBlendingOut = false;

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
	void UpdateNotifyOverride(float DeltaTime);
	void ApplyToSpringArm();
	void UpdateFallFeel(float DeltaTime);


	// first person camera system
	// In header — private
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Camera")
	UCameraComponent* FPSCameraRef = nullptr;

	bool bIsFPSActive = false;

	void EvaluateAndInterpFPS(float DeltaTime);
	void SwitchToFPS(bool bEnable);

	void ConfigureFPSViewLimits(bool bIsProne);
	void ResetCameraManagerLimits();
};