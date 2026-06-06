#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AISightTargetInterface.h" 
#include "LocomotionComponent.h"
#include "LocomotionInterface.h"
#include "StealthSystem.h"
#include "ActionComponent.h"
#include "BaseCharacter.generated.h"

UCLASS()
class LOCOMOTIONSYSTEM_API ABaseCharacter : public ACharacter, public ILocomotionInterface, public IAISightTargetInterface
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UActionComponent* ActionComponent = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UActorComponent* AC_Hitbox = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UActorComponent* AC_HitReaction = nullptr;


	// AI variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Variables")
	TArray<FName> SocketsToCheckForSight = {"head", "spine_03", "spine_04", "pelvis", "hand_l", "hand_r", "thigh_l", "thigh_r"};


	/** Internal routine to force stable, interactive ragdoll corpse states natively */
	UFUNCTION(BlueprintNativeEvent, Category = "Infiltration Core | Serialization Internal")
	void InitializeCorpseState(const FStruct_NPCDataAssetPayload& Payload);
	virtual void InitializeCorpseState_Implementation(const FStruct_NPCDataAssetPayload& Payload);

	/** Internal routine to initialize living AI behavior configurations cleanly */
	UFUNCTION(BlueprintNativeEvent, Category = "Infiltration Core | Serialization Internal")
	void InitializeLivingState(const FStruct_NPCDataAssetPayload& Payload);
	virtual void InitializeLivingState_Implementation(const FStruct_NPCDataAssetPayload& Payload);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	// stealth system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infiltration Core | Identity")
	FStruct_InfiltrationProfile InfiltrationProfile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infiltration Core | Identity")
	FString NPCIdentityName = TEXT("Unnamed Guard");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infiltration Core | Guard Settings")
	TMap<FName, float> OutfitSuspicionMultipliers;
	/** Overridable event driven straight out of Easy Game UI's Load game variables delegate pass */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Infiltration Core | Serialization")
	void PostLoadInitialization(const FStruct_NPCDataAssetPayload& LoadedPayload);
	virtual void PostLoadInitialization_Implementation(const FStruct_NPCDataAssetPayload& LoadedPayload);

	/** Prepares a fresh, living character layout structure to write directly to Easy Game UI's Save script macro */
	UFUNCTION(BlueprintPure, Category = "Infiltration Core | Serialization")
	FStruct_NPCDataAssetPayload PackageSavePayload() const;


	// locomotion overrides
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	UFUNCTION(BlueprintNativeEvent, Category = "Locomotion Overrides")
	void UpdateMovement();
	UFUNCTION(BlueprintNativeEvent, Category = "Locomotion Overrides")
	void UpdateRotation();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Locomotion Overrides")
	FStruct_CharacterInputState GetCurrentInputState() const;
	virtual void CallUpdateMovement_Implementation() override;
	virtual void CallUpdateRotation_Implementation() override;



	// perception overrides
	// override where the enemy can detect the character from, we will check multiple sockets to see if any of them are visible to the observer
	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor = nullptr, const bool* bWasVisible = nullptr, int32* UserData = nullptr) const;
	// override the ai perception component location and rotation to be from the character's head instead of the actor's origin
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
	UFUNCTION(BlueprintNativeEvent, Category = "AI Variables | Perception")
	void GetPerceptionLocationAndRotation(FVector& OutLocation, FRotator& OutRotation) const;

};
