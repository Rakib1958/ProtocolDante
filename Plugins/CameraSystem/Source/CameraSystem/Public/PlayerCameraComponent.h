#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Camera|Init") void SetCameraReference();
	UFUNCTION(BlueprintCallable, Category = "Camera|Init") void InitCamera(bool bUseGameplayCamera);

	// ── Preset Maps ───────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets") TMap<FGameplayTag, FStruct_CameraRigParams> BasePresetMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets") TMap<FGameplayTag, FStruct_CameraRigOffset> StanceOffsetMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets") TMap<FGameplayTag, FStruct_CameraRigOffset> ShoulderOffsetMap;

	// ── Runtime State ─────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadWrite, Category = "Camera|State")
	FStruct_SpringArmCamera DesiredCameraState;

	// True while a notify override is active (blending in or holding)
	// Also checked by Blueprint notify state to guard FPS ignoring
	UPROPERTY(BlueprintReadOnly, Category = "Camera|State") bool bIsOverrideActive = false;
	UPROPERTY(BlueprintReadOnly, Category = "Camera|State") bool bIsFPSActive = false;

	// ── Notify Override Interface — called from ANS_CameraOverride ────────────
	UFUNCTION(BlueprintCallable, Category = "Camera|Notify")
	void BeginCameraOverride(
		FVector TargetSocketOffset, FRotator TargetRotation, float TargetFOVDelta,
		float InterpInSpeed,
		bool bUseCustomDefault,
		FVector DefaultSocketOffset, FRotator DefaultRotation, float DefaultFOVDelta,
		float InterpOutSpeed,
		bool bForceTPS,
		bool bIgnoreIfFPS
	);

	// NotifyEnd calls this.
	UFUNCTION(BlueprintCallable, Category = "Camera|Notify") void EndCameraOverride();

	// NotifyTick calls this — drives the interpolation each frame.
	UFUNCTION(BlueprintCallable, Category = "Camera|Notify") void UpdateNotifyOverride(float DeltaTime);

	// ── First Person ──────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Camera|FirstPerson") void SwitchToFPS(bool bEnable);

	// ── Default Values ────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Defaults") float TargetArmLength = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Defaults") FVector SpringArmLocation = FVector(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Defaults") FRotator SpringArmRotation = FRotator(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Defaults") float LagSpeed = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Defaults") float RotationLagSpeed = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Defaults") float FieldOfView = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Defaults") float CrouchZInterpSpeed = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Defaults") float FOV_Multiplier = 1.f;

protected:
	// ── First Person Config ───────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson") FVector FPSMeshOffset = FVector(15.f, 0.f, 5.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson") float FPSNormalPitchLimit = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson") float FPSPronePitchLimit = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FirstPerson") float FPSProneRotationLag = 3.f;

private:
	// ── References ────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true")) ACharacter* Character = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true")) UCharacterMovementComponent* CharacterMovement = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true")) USpringArmComponent* SpringArmRef = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true")) UCameraComponent* CameraRef = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true")) UCameraComponent* FPSCameraRef = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "GameplayCamera", meta = (AllowPrivateAccess = "true")) UGameplayCameraComponent* GameplayCameraRef = nullptr;

	// ── Interpolated Layers ───────────────────────────────────────────────────

	FStruct_CameraRigParams CurrentBase;
	FStruct_CameraRigOffset CurrentStanceOffset;
	FStruct_CameraRigOffset CurrentShoulderOffset;

	// ── Notify Override — raw fields, no struct ───────────────────────────────

	// Target values (blend toward on begin)
	FVector  NotifyTargetSocketOffset = FVector::ZeroVector;
	FRotator NotifyTargetRotation = FRotator::ZeroRotator;
	float    NotifyTargetFOVDelta = 0.f;
	float    NotifyInterpInSpeed = 5.f;

	// Default/restore values (blend toward on end)
	FVector  NotifyDefaultSocketOffset = FVector::ZeroVector;
	FRotator NotifyDefaultRotation = FRotator::ZeroRotator;
	float    NotifyDefaultFOVDelta = 0.f;
	float    NotifyInterpOutSpeed = 3.f;

	// Live interpolated values — applied additively in ApplyToSpringArm
	FVector  CurrentNotifySocketOffset = FVector::ZeroVector;
	FRotator CurrentNotifyRotation = FRotator::ZeroRotator;
	float    CurrentNotifyFOVDelta = 0.f;

	bool bNotifyBlendingIn = false;
	bool bNotifyBlendingOut = false;
	bool bForcedTPSFromFPS = false;
	bool bIgnoreThisNotify = false;

	// ── Stance Z Smoothing ────────────────────────────────────────────────────

	FGameplayTag LastStanceOffset;
	bool  bIsStanceTransitioning = false;
	float SmoothedPivotZ = 0.f;

	// ── Fall Feel ─────────────────────────────────────────────────────────────
	float FallFOVReduction = 0.f;
	float FallArmReduction = 0.f;

	// ── Private Methods ───────────────────────────────────────────────────────
	void SetReference();
	void CreateCamera();
	void InitializeCamera();
	void CreateGameplayCamera();
	void InitializeGameplayCamera();

	void EvaluateAndInterpBase(float DeltaTime);
	void EvaluateAndInterpStanceOffset(float DeltaTime);
	void EvaluateAndInterpShoulderOffset(float DeltaTime);
	void UpdateCapsuleZSmoothing(float DeltaTime);
	void UpdateFallFeel(float DeltaTime);
	void EvaluateAndInterpFPS(float DeltaTime);
	void ApplyToSpringArm();

	void ConfigureFPSViewLimits(bool bIsProne);
	void ResetCameraManagerLimits();
};