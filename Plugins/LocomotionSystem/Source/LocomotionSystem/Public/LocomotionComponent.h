#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Curves/CurveFloat.h"
#include "Components/CapsuleComponent.h"
#include "LocomotionComponent.generated.h"

// ── Enumerations ──────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class Enum_MovementMode : uint8
{
	OnGround UMETA(DisplayName = "On Ground"),
	InAir    UMETA(DisplayName = "In Air"),
	Ragdoll  UMETA(DisplayName = "Ragdoll")
};

UENUM(BlueprintType)
enum class Enum_MovementState : uint8
{
	Idle   UMETA(DisplayName = "Idle"),
	Moving UMETA(DisplayName = "Moving")
};

UENUM(BlueprintType)
enum class Enum_CharacterState : uint8
{
	Relaxed UMETA(DisplayName = "Relaxed"),
	Stealth UMETA(DisplayName = "Stealth"),
	Combat  UMETA(DisplayName = "Combat")
};

UENUM(BlueprintType)
enum class Enum_Stance : uint8
{
	Stand  UMETA(DisplayName = "Stand"),
	Crouch UMETA(DisplayName = "Crouch"),
	Prone  UMETA(DisplayName = "Prone")
};

UENUM(BlueprintType)
enum class Enum_Gait : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Jog  UMETA(DisplayName = "Jog"),
	Run  UMETA(DisplayName = "Run")
};

// ── Input State Struct ────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FStruct_CharacterInputState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToRun = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToWalk = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToStrafe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToAim = false;
};

// ── Delegates ─────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateMovement);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateRotation);

// ── Component ─────────────────────────────────────────────────────────────────

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

	// ── Private ───────────────────────────────────────────────────────────────────
private:
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

	void SetReferences();
	bool HasMovementInputVector();
	bool CanSprint() const;

	float CalculateMaxAcceleration();
	float CalculateMaxBrakingDeceleration();
	float CalculateGroundFriction();
	float CalculateMaxSpeed();
	float CalculateMaxSpeedCrouched();
	float CalculateMaxSpeedProne();
	Enum_Gait GetDesiredGait();

	// Stance transition — smooth capsule height interpolation
	void TickStanceTransition(float DeltaTime);
	float TargetCapsuleHalfHeight = 88.f;
	float ProneRotationRateYaw = 40.f;
	float CurrentProneRotationRate = 360.f;

	// ── Protected Config ──────────────────────────────────────────────────────────
protected:
	// Ragdoll data — kept as BlueprintImplementableEvent contract only
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

	// Curves
	UPROPERTY(EditAnywhere, Category = "Curves")
	UCurveFloat* StrafeSpeedMapCurve = nullptr;

	// Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector RunSpeed = FVector(500.f, 500.f, 500.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector JogSpeed = FVector(375.f, 375.f, 375.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector CrouchSpeed = FVector(225.f, 200.f, 180.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector ProneSpeed = FVector(80.f, 80.f, 80.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector WalkSpeedRelaxed = FVector(165.f, 165.f, 165.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector WalkSpeedStealth = FVector(165.f, 165.f, 165.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector WalkSpeedCombat = FVector(165.f, 165.f, 165.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed")
	FVector LandVelocity = FVector::ZeroVector;

	// Acceleration / Friction
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
	float ProneGroundFriction = 10.f;

	// Capsule heights
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Capsule")
	float StandCapsuleHalfHeight = 88.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Capsule")
	float CrouchCapsuleHalfHeight = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Capsule")
	float ProneCapsuleHalfHeight = 30.f;

	// How fast the capsule interpolates between heights
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Capsule")
	float StanceTransitionSpeed = 10.f;

	// Prone rotation — controlled turn rate so prone doesn't snap
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Prone")
	float ProneRotationRate = 40.f;

	// How fast rotation rate interpolates into/out of prone value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Prone")
	float ProneRotationInterpSpeed = 5.f;

	// ── Public ────────────────────────────────────────────────────────────────────
public:
	// States
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	FStruct_CharacterInputState CharacterInputState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	bool bJustLanded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	Enum_MovementMode MovementMode = Enum_MovementMode::OnGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	Enum_MovementState MovementState = Enum_MovementState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	Enum_CharacterState CharacterState = Enum_CharacterState::Relaxed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	Enum_Stance Stance = Enum_Stance::Stand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	Enum_Gait Gait = Enum_Gait::Jog;

	// Ragdoll runtime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	float FlailRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	UAnimSequence* FlailSequence = nullptr;

	// Delegates
	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates")
	FOnUpdateMovement OnUpdateMovement;

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates")
	FOnUpdateRotation OnUpdateRotation;

	// Ragdoll — implemented entirely in Blueprint
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Ragdoll")
	void StartRagdoll();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Ragdoll")
	void StopRagdoll();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Ragdoll")
	void UpdateRagdoll();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Ragdoll")
	void SetActorLocationDuringRagdoll();

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintImplementableEvent, Category = "Ragdoll")
	UAnimMontage* GetRagdollGetUpMontage();

	// Update
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "States")
	bool IsSprinting();

	UFUNCTION(BlueprintCallable, Category = "Update Data")
	void UpdateGait();

	UFUNCTION(BlueprintCallable, Category = "Update Data")
	void UpdateDynamicMovementSettings();

	// Set state
	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetStance(Enum_Stance NewStance);

	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetProne(bool bWantsProne);

	UFUNCTION(BlueprintPure, Category = "Set State")
	bool CanStandFromProne() const;

	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetMovementMode(Enum_MovementMode NewMovementMode);

	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetCharacterState(Enum_CharacterState NewCharacterState);

	// Input hooks
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input")
	void WantsToRun(bool bStarted);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input")
	void WantsToAim(bool bStarted);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input")
	void WantsToWalk(bool bStarted);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input")
	void WantsToStrafe();
};