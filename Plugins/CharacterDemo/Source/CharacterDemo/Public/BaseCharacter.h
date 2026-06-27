#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AISightTargetInterface.h" 
#include "BaseCharacter.generated.h"

class ULocomotionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParkourFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTraversalFinished);

UCLASS()
class CHARACTERDEMO_API ABaseCharacter : public ACharacter, public IAISightTargetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	ULocomotionComponent* LocomotionComponent = nullptr;

	// AI variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Variables")
	TArray<FName> SocketsToCheckForSight = { "head", "spine_03", "spine_04", "pelvis", "hand_l", "hand_r", "thigh_l", "thigh_r" };

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// locomotion overrides
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	// perception overrides
		// override where the enemy can detect the character from, we will check multiple sockets to see if any of them are visible to the observer
	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor = nullptr, const bool* bWasVisible = nullptr, int32* UserData = nullptr) const;
	// override the ai perception component location and rotation to be from the character's head instead of the actor's origin
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
	UFUNCTION(BlueprintNativeEvent, Category = "AI Variables | Perception")
	void GetPerceptionLocationAndRotation(FVector& OutLocation, FRotator& OutRotation) const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Input")
	FStruct_CharacterInputState GetCurrentInputState();
	
// variables and functions needed in blueprint override
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure,  BlueprintCallable, Category = "Combat") FRotator GetLockOnTargetRotation() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = "Combat") bool IsDefending() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = "Combat") bool CanActivateSoftLock() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = "Combat") bool NoCombatActive() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat") void SoftLockRotation();
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = "Parkour") bool NoParkourActive() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = "Parkour") bool NoTraversalActive() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = "Parkour") bool CanDoTraversal() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = "Parkour") bool JustFinishedParkour() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Parkour") void TryToParkour() ;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Interact") void TryToInteract(bool Pressed) ;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Locomotion") void UpdateMovement();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Locomotion") void UpdateRotation();
};
