#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Curves/CurveFloat.h"
#include "Components/CapsuleComponent.h"
#include "LocomotionComponent.generated.h"

// enumerations
UENUM(BlueprintType)
enum class Enum_MovementMode : uint8 {
	OnGround UMETA(DisplayName = "On Ground"),
	InAir UMETA(DisplayName = "In Air"),
	Ragdoll UMETA(DisplayName = "Ragdoll")
};
UENUM(BlueprintType)
enum class Enum_MovementState : uint8 {
	Idle UMETA(DisplayName = "Idle"),
	Moving UMETA(DisplayName = "Moving")
};
UENUM(BlueprintType)
enum class Enum_CharacterState : uint8 {
	Relaxed UMETA(DisplayName = "Relaxed"),
	Stealth UMETA(DisplayName = "Stealth"),
	Combat UMETA(DisplayName = "Combat")
};
UENUM(BlueprintType)
enum class Enum_Stance : uint8 {
	Stand UMETA(DisplayName = "Stand"),
	Crouch UMETA(DisplayName = "Crouch")
};
UENUM(BlueprintType)
enum class Enum_Gait : uint8 {
	Walk UMETA(DisplayName = "Walk"),
	Jog UMETA(DisplayName = "Jog"),
	Run UMETA(DisplayName = "Run")
};

// structures
USTRUCT(BlueprintType)
struct FStruct_CharacterInputState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToSprint = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToWalk = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToStrafe = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WantsToAim = false;
};

// delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateMovement);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateRotation);

UCLASS( ClassGroup=(Custom), Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent) )
class LOCOMOTIONSYSTEM_API ULocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

// default functions
public:	
	// Sets default values for this component's properties
	ULocomotionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
// custom logic
private:
	// private variables
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	ACharacter* Character;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UCharacterMovementComponent* CharacterMovement;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleComponent;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	UAnimInstance* MainAnimInstance;
	UPROPERTY(BlueprintReadOnly, Category = "References", meta = (AllowPrivateAccess = "true"))
	bool bIsValidCharacter = false;

	void SetReferences();
	bool HasMovementInputVector();
	bool CanSprint() const;
	float CalculateMaxAcceleration();
	float CalculateMaxBrakingDeceleration();
	//float CalculateBrakingFriction();
	float CalculateGroundFriction();
	float CalculateMaxSpeed();
	float CalculateMaxSpeedCrouched();
	Enum_Gait GetDesiredGait();

protected:
	// ragdoll
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
	FVector LastRagdollVelocity = FVector(0.f, 0.f, 0.f);

	// curves
	UPROPERTY(EditAnywhere, Category = "Curves")
	UCurveFloat* StrafeSpeedMapCurve;



	// speed map
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector RunSpeed = FVector(500.f, 500.f, 500.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector JogSpeed = FVector(375.f, 375.f, 375.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector CrouchSpeed = FVector(225.f, 200.f, 180.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector WalkSpeedRelaxed = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector WalkSpeedStealth = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector WalkSpeedCombat = FVector(165.f, 165.f, 165.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector LandVelocity = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float WalkAcceleration = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float RunAcceleration = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float BrakingDeceleration = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float WalkGroundFriction = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float RunGroundFriction = 3.f;
	

public:

	// variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	FStruct_CharacterInputState CharacterInputState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States")
	bool bJustLanded = false;

	// states
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

	//ragdoll
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	float FlailRate = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll")
	UAnimSequence* FlailSequence = nullptr;



	// delegates
	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates")
	FOnUpdateMovement OnUpdateMovement;
	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates")
	FOnUpdateRotation OnUpdateRotation;

	// public functions
	
	// ragdoll overridable functions
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

	// update data
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "States")
	bool IsSprinting();
	UFUNCTION(BlueprintCallable, Category = "Update Data")
	void UpdateGait();
	UFUNCTION(BlueprintCallable, Category = "Update Data")
	void UpdateDynamicMovementSettings();

	// set state
	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetStance(Enum_Stance NewStance);
	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetMovementMode(Enum_MovementMode NewMovementMode);
	UFUNCTION(BlueprintCallable, Category = "Set State")
	void SetCharacterState(Enum_CharacterState NewCharacterState);

	// input events
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input")
	void WantsToSprint(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input")
	void WantsToAim(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input")
	void WantsToWalk(bool bStarted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Input")
	void WantsToStrafe();

};
