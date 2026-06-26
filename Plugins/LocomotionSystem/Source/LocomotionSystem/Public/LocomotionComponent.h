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
	Run           UMETA(DisplayName = "Run"),
	Sprint           UMETA(DisplayName = "Sprint")
};
UENUM(BlueprintType)
enum class Enum_RotationMode : uint8 {
	VelocityDirection UMETA(DisplayName = "Velocity Direction"),
	LookingDirection  UMETA(DisplayName = "Looking Direction")
};

// ─── INPUT TRACKING STATE STRUCT ───
USTRUCT(BlueprintType)
struct FStruct_CharacterInputState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion | Input")
	bool WantsToSprint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion | Input")
	bool WantsToWalk = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion | Input")
	bool WantsToStrafe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion | Input")
	bool WantsToAim = false;
};

// ─── SYSTEM MULTI-CAST DELEGATES ───
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateMovementSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateRotationSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRagdollEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStanceChanged, Enum_Stance, NewStance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGaitChanged, Enum_Gait, NewGait);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementModeChanged, Enum_MovementMode, NewMovementMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChanged, Enum_CharacterState, NewCharacterState);

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
	// Core Cached Native Pointers
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true")) ACharacter* Character = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true")) UCharacterMovementComponent* CharacterMovement = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true")) UCapsuleComponent* CapsuleComponent = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true")) USkeletalMeshComponent* Mesh = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true")) UAnimInstance* MainAnimInstance = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true")) bool bIsValidCharacter = false;

	// Internal Computation Subroutines
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

	// Smooth Dimension Transitions & Early Fall Predictor
	void TickStanceTransition(float DeltaTime);
	void CheckPredictiveProneLedgeFall();
	void SetRotationWhileProning(float DeltaTime);
	float GetGaitScore(Enum_Gait InGait) const;

	float TargetCapsuleHalfHeight = 0.f;
	//void DebugPrint(FString text, FColor color, float duration);

	/** Sweeps forward and backward to prevent head and feet from clipping through geometry. */
	void HandleProneExtremityCollisions(float DeltaTime);


protected:
	// ─── RAGDOLL STORAGE PARAMETERS ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll") UAnimMontage* RagdollGetUpBack = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll") UAnimMontage* RagdollGetUpFront = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll") FName PelvisBone = "pelvis";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll") FName SnapshotName = "RagdollPose";
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll|Do Not Change") bool bRagdollOnGround = false;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll|Do Not Change") bool bRagdollFaceUp = false;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll|Do Not Change") FVector LastRagdollVelocity = FVector::ZeroVector;

	// ─── EVALUATION CURVES ───
	UPROPERTY(EditAnywhere, Category = "Curves") UCurveFloat* StrafeSpeedMapCurve = nullptr;

	// ─── BALANCING CONFIGURATION TUNERS ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed") FVector SprintSpeed = FVector(500.f, 500.f, 500.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed") FVector RunSpeed = FVector(375.f, 375.f, 375.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed") FVector CrouchSpeed = FVector(225.f, 200.f, 180.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed") FVector ProneSpeed = FVector(80.f, 60.f, 50.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed") FVector WalkSpeedRelaxed = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed") FVector WalkSpeedStealth = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed") FVector WalkSpeedCombat = FVector(165.f, 165.f, 165.f);
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Speed|Do Not Change") FVector LandVelocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Acceleration") float WalkAcceleration = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Acceleration") float RunAcceleration = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Acceleration") float BrakingDeceleration = 500.f;
	/** Maximum input acceleration driving force when crawling completely straight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Acceleration") float ProneMaxAcceleration = 350.f;
	/** Minimum input acceleration driving force when executing maximum, heavy turning arcs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Acceleration") float ProneMinAcceleration = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Friction") float WalkGroundFriction = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Friction") float RunGroundFriction = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Friction") float ProneGroundFriction = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Dimensions") float StandCapsuleHalfHeight = 88.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Dimensions") float CrouchCapsuleHalfHeight = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Dimensions") float ProneCapsuleHalfHeight = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Dimensions") float StanceTransitionSpeed = 10.f;
	// How fast the character Moves by default while proning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Rotation") float ProneRotationRateYaw = 100.f;
	// Maximum turning rate when moving straight or making microscopic corrections
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Rotation") float ProneMaxRotationRateYaw = 100.f;

	// Minimum turning rate when executing heavy, wide turning arcs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Rotation") float ProneMinRotationRateYaw = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Rotation") float ProneRotationInterpSpeed = 7.5f;   // Main "struggle / inertia" control

	/** Distance from the capsule center to the tip of the character's head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Collision")
	float ProneHeadCheckLength = 85.f;

	/** Distance from the capsule center to the tip of the character's feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Collision")
	float ProneLegCheckLength = 95.f;

	/** Thickness radius of the body check (should approximate shoulder/waist width). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Collision")
	float ProneExtremityRadius = 24.f;

public:
	// Dynamic Runtime States Tracking, you can set by default WantsToStrafe for NPCs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States") FStruct_CharacterInputState CharacterInputState;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Do Not Change") bool bJustLanded = false;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States|Do Not Change") Enum_MovementMode MovementMode = Enum_MovementMode::OnGround;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States|Do Not Change") Enum_MovementState MovementState = Enum_MovementState::Idle;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States|Do Not Change") Enum_CharacterState CharacterState = Enum_CharacterState::Relaxed;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States|Do Not Change") Enum_Stance Stance = Enum_Stance::Stand;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States|Do Not Change") Enum_Gait Gait = Enum_Gait::Run;
	// Do Not Modify these values, these are changed by the functions associated with it
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "States|Do Not Change") Enum_Gait MaxGait = Enum_Gait::Sprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll|Do Not Change") float FlailRate = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll") UAnimSequence* FlailSequence = nullptr;

	// Dispatcher Event Triggers
	UPROPERTY(BlueprintAssignable, Category = "Delegates") FOnUpdateMovementSignature OnUpdateMovement;
	UPROPERTY(BlueprintAssignable, Category = "Delegates") FOnUpdateRotationSignature OnUpdateRotation;
	UPROPERTY(BlueprintAssignable, Category = "Delegates") FOnRagdollEnd OnRagdollEnd;
	UPROPERTY(BlueprintAssignable, Category = "Delegates") FOnStanceChanged OnStanceChanged;
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Delegates") FOnMovementModeChanged OnMovementModeChanged;
	UPROPERTY(BlueprintAssignable, Category = "Delegates")  FOnCharacterStateChanged OnCharacterStateChanged;
	UPROPERTY(BlueprintAssignable, Category = "Delegates") FOnGaitChanged OnGaitChanged;
	
	// ─── PUBLIC BLUEPRINT HANDSHAKES ───
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ragdoll") void StartRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ragdoll") void StopRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ragdoll") void UpdateRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ragdoll") void SetActorLocationDuringRagdoll();
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintNativeEvent, Category = "Ragdoll") UAnimMontage* GetRagdollGetUpMontage() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "States") bool IsSprinting();
	UFUNCTION(BlueprintCallable, Category = "Update Data") void UpdateGait();
	UFUNCTION(BlueprintCallable, Category = "Update Data") void UpdateDynamicMovementSettings();

	UFUNCTION(BlueprintCallable, Category = "Set State") void SetStance(Enum_Stance NewStance);
	UFUNCTION(BlueprintCallable, Category = "Set State") void SetMovementMode(Enum_MovementMode NewMovementMode);
	UFUNCTION(BlueprintCallable, Category = "Set State") void SetCharacterState(Enum_CharacterState NewCharacterState);
	UFUNCTION(BlueprintCallable, Category = "Set State") void SetProne(bool bWantsToProne);
	UFUNCTION(BlueprintPure, Category = "Queries") bool CanStandFromProne() const;
	UFUNCTION(BlueprintPure, Category = "Queries") bool CanStandFromCrouch() const;

	// ─── NATIVE INPUT EXECUTION HOOKS ───
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input") void WantsToSprint(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input") void WantsToAim(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input") void WantsToWalk(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input") void WantsToStrafe();

	FVector GetLandVelocity() const;
};