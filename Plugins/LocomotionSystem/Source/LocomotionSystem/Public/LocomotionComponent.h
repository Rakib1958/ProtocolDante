#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Curves/CurveFloat.h"
#include "Components/CapsuleComponent.h"
#include "LocomotionComponent.generated.h"

// ─── ENUMERATIONS MATRIX ───
UENUM(BlueprintType)
enum class Enum_MovementMode : uint8 {
	OnGround      UMETA(DisplayName = "On Ground"),
	InAir         UMETA(DisplayName = "In Air"),
	Ragdoll       UMETA(DisplayName = "Ragdoll")
};

UENUM(BlueprintType)
enum class Enum_MovementState : uint8 {
	Idle          UMETA(DisplayName = "Idle"),
	Moving        UMETA(DisplayName = "Moving")
};

UENUM(BlueprintType)
enum class Enum_CharacterState : uint8 {
	Relaxed       UMETA(DisplayName = "Relaxed"),
	Stealth       UMETA(DisplayName = "Stealth"),
	Combat        UMETA(DisplayName = "Combat")
};

UENUM(BlueprintType)
enum class Enum_Stance : uint8 {
	Stand         UMETA(DisplayName = "Stand"),
	Crouch        UMETA(DisplayName = "Crouch"),
	Prone         UMETA(DisplayName = "Prone")
};

UENUM(BlueprintType)
enum class Enum_Gait : uint8 {
	Walk          UMETA(DisplayName = "Walk"),
	Jog           UMETA(DisplayName = "Jog"),
	Run           UMETA(DisplayName = "Run")
};

UENUM(BlueprintType)
enum class Enum_RotationMode : uint8 {
	VelocityDirection UMETA(DisplayName = "Velocity Direction"),
	LookingDirection  UMETA(DisplayName = "Looking Direction")
};

// ─── STRUCTS ───
USTRUCT(BlueprintType)
struct FStruct_CharacterInputState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion | Input")
	bool WantsToRun = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion | Input")
	bool WantsToWalk = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion | Input")
	bool WantsToStrafe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion | Input")
	bool WantsToAim = false;
};

// ─── MULTI-CAST DELEGATES ───
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateMovementSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateRotationSignature);

UCLASS(ClassGroup = (Custom), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOCOMOTIONSYSTEM_API ULocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULocomotionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Core Cached References
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	ACharacter* Character = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UCharacterMovementComponent* CharacterMovement = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UAnimInstance* MainAnimInstance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	bool bIsValidCharacter = false;

	// Internal Subroutines
	void SetReferences();
	bool HasMovementInputVector();
	bool CanSprint() const;

	float CalculateMaxAcceleration();
	float CalculateMaxBrakingDeceleration();
	float CalculateGroundFriction();
	float CalculateMaxSpeed();
	float CalculateMaxSpeedCrouched();
	float CalculateMaxSpeedProned();
	Enum_Gait GetDesiredGait();

	// Smooth Stance Transition System Parameters
	void TickStanceTransition(float DeltaTime);
	void TickProneAlignment(float DeltaTime);
	float TargetCapsuleHalfHeight = 0.f;
	float CurrentProneIdleOffset = 0.f;

	bool bLedgeCheckPending = false;

	void TickProneLedgeCheck(float DeltaTime);
	void TickFloorDetection(float DeltaTime);

protected:
	// ─── RAGDOLL PARAMETERS ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	UAnimMontage* RagdollGetUpBack = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	UAnimMontage* RagdollGetUpFront = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FName PelvisBone = "pelvis";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FName SnapshotName = "RagdollPose";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	bool bRagdollOnGround = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	bool bRagdollFaceUp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	FVector LastRagdollVelocity = FVector::ZeroVector;

	// ─── CURVES ───
	UPROPERTY(EditAnywhere, Category = "Curves")
	UCurveFloat* StrafeSpeedMapCurve = nullptr;

	// ─── CONFIG TUNING MATRIX ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector RunSpeed = FVector(500.f, 500.f, 500.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector JogSpeed = FVector(375.f, 375.f, 375.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector CrouchSpeed = FVector(225.f, 200.f, 180.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector ProneSpeed = FVector(80.f, 60.f, 50.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector WalkSpeedRelaxed = FVector(165.f, 165.f, 165.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector WalkSpeedStealth = FVector(165.f, 165.f, 165.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector WalkSpeedCombat = FVector(165.f, 165.f, 165.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector LandVelocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Acceleration")
	float WalkAcceleration = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Acceleration")
	float RunAcceleration = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Acceleration")
	float BrakingDeceleration = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Friction")
	float WalkGroundFriction = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Friction")
	float RunGroundFriction = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Friction")
	float ProneGroundFriction = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Half Height")
	float StandCapsuleHalfHeight = 88.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Half Height")
	float ProneCapsuleHalfHeight = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Half Height")
	float CrouchCapsuleHalfHeight = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Half Height")
	float StanceTransitionSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Half Height")
	float ProneIdleMeshZOffset = -22.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Half Height")
	float FootPlacementRootOffset = -4.f; // Tune this — typically -2 to -6

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Prone")
	float SlopeAlignmentInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Prone")
	float SlopeTraceDistance = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Prone")
	float LedgeCheckDistance = 40.f;   // How far forward to probe

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Prone")
	float LedgeCheckDepth = 80.f;      // How far down to look for ground

	UPROPERTY(BlueprintReadOnly, Category = "Prone")
	float ProneSlopePitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Prone")
	float ProneSlopeRoll = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Prone")
	float FloorAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Prone")
	float DynamicPelvisMaxOffset = 250.f;

	UPROPERTY(BlueprintReadOnly, Category = "Prone")
	float DynamicPelvisStiffness = 100.f;

	float ProneSettleTimer = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Prone")
	float ProneSettleDelay = 0.4f;

	

	FRotator CurrentMeshSlopeOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Rotation")
	float ProneRotationRateYaw = 35.f; // ◄ Controlled turning rate parameter for prone movement

public:
	// Dynamic Tracking Variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States") FStruct_CharacterInputState CharacterInputState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States") bool bJustLanded = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States") Enum_MovementMode MovementMode = Enum_MovementMode::OnGround;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States") Enum_MovementState MovementState = Enum_MovementState::Idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States") Enum_CharacterState CharacterState = Enum_CharacterState::Relaxed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States") Enum_Stance Stance = Enum_Stance::Stand;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States") Enum_Gait Gait = Enum_Gait::Jog;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll") float FlailRate = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll") UAnimSequence* FlailSequence = nullptr;

	// Dispatcher Triggers
	UPROPERTY(BlueprintAssignable, Category = "Delegates") FOnUpdateMovementSignature OnUpdateMovement;
	UPROPERTY(BlueprintAssignable, Category = "Delegates") FOnUpdateRotationSignature OnUpdateRotation;

	// ─── PUBLIC BLUEPRINT IMPLEMENTATIONS ───
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Ragdoll") void StartRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Ragdoll") void StopRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Ragdoll") void UpdateRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Ragdoll") void SetActorLocationDuringRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintImplementableEvent, Category = "Ragdoll") UAnimMontage* GetRagdollGetUpMontage();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "States") bool IsSprinting();
	UFUNCTION(BlueprintCallable, Category = "Update Data") void UpdateGait();
	UFUNCTION(BlueprintCallable, Category = "Update Data") void UpdateDynamicMovementSettings();

	UFUNCTION(BlueprintCallable, Category = "Set State") void SetStance(Enum_Stance NewStance);
	UFUNCTION(BlueprintCallable, Category = "Set State") void SetMovementMode(Enum_MovementMode NewMovementMode);
	UFUNCTION(BlueprintCallable, Category = "Set State") void SetCharacterState(Enum_CharacterState NewCharacterState);
	UFUNCTION(BlueprintCallable, Category = "Set State") void SetProne(bool bWantsToProne);
	UFUNCTION(BlueprintPure, Category = "Queries") bool CanStandFromProne() const;

	// ─── INPUT HOOK OVERRIDES ───
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input") void WantsToRun(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input") void WantsToAim(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input") void WantsToWalk(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input") void WantsToStrafe();
};