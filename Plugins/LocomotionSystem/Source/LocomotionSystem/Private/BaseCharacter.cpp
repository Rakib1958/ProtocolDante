// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "AIController.h"

// Sets default values
ABaseCharacter::ABaseCharacter()
{
	// Set this character to call Tick() every frame.  If he'd dead we can skip ticking the character, improving performance.
	PrimaryActorTick.bCanEverTick = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true; // enable crouching for this character
    GetCharacterMovement()->NavAgentProps.bCanFly = true;
	GetCharacterMovement()->NavAgentProps.bCanSwim = true;
    GetCharacterMovement()->bCanWalkOffLedges = true;
	GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, -1.f, 0.f);
	LocomotionComponent = CreateDefaultSubobject<ULocomotionComponent>(TEXT("LocomotionComponent"));
    ActionComponent = CreateDefaultSubobject<UActionComponent>(TEXT("ActionComponent"));
    AC_Hitbox = CreateDefaultSubobject<UActorComponent>(TEXT("AC_Hitbox"));
    AC_HitReaction = CreateDefaultSubobject<UActorComponent>(TEXT("AC_HitReaction"));
}

void ABaseCharacter::CallUpdateMovement_Implementation()
{
    UpdateMovement();
}
void ABaseCharacter::CallUpdateRotation_Implementation()
{
    UpdateRotation();
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ABaseCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	LocomotionComponent->SetStance(Enum_Stance::Crouch);
}
void ABaseCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	LocomotionComponent->SetStance(Enum_Stance::Stand);
}
void ABaseCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	switch (GetCharacterMovement()->MovementMode)
	{
	case EMovementMode::MOVE_Walking:
		LocomotionComponent->SetMovementMode(Enum_MovementMode::OnGround);
		break;
	case EMovementMode::MOVE_Falling:
		LocomotionComponent->SetMovementMode(Enum_MovementMode::InAir);
		break;
	case EMovementMode::MOVE_NavWalking:
		LocomotionComponent->SetMovementMode(Enum_MovementMode::OnGround);
		break;
	default:
		break;
	}
}

void ABaseCharacter::UpdateMovement_Implementation()
{
    // this is implementation based on a gasp character...can be overriden to a lyra or als type character in blueprint too...
    LocomotionComponent->UpdateGait();
    LocomotionComponent->UpdateDynamicMovementSettings();
}
void ABaseCharacter::UpdateRotation_Implementation()
{
    // this is implementation based on a gasp character...can be overriden to a lyra or als type character in blueprint too...
    if (GetCurrentInputState().WantsToAim || GetCurrentInputState().WantsToStrafe)
    {
        GetCharacterMovement()->bOrientRotationToMovement = false;
        GetCharacterMovement()->bUseControllerDesiredRotation = true;
    }
    else
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
        GetCharacterMovement()->bUseControllerDesiredRotation = false;
	}
    if (GetCharacterMovement()->IsFalling())
    {
        GetCharacterMovement()->RotationRate = FRotator(0.f, 2000.f, 0.f); 
    }
    else
    {
        GetCharacterMovement()->RotationRate = FRotator(0.f, -1.f, 0.f);
    }
}
FStruct_CharacterInputState ABaseCharacter::GetCurrentInputState() const
{
    return LocomotionComponent->CharacterInputState;
}

// perception
void ABaseCharacter::GetPerceptionLocationAndRotation_Implementation(FVector& OutLocation, FRotator& OutRotation) const
{
    // For simplicity, we'll just use the actor's location and rotation. You can customize this to return specific sockets or offsets if needed.
    OutLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
    OutRotation = GetActorRotation();
}
bool ABaseCharacter::CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const
{

    FCollisionQueryParams QueryParams(FName(TEXT("MultiPointLineOfSight")), true);
    QueryParams.AddIgnoredActor(IgnoreActor);
    QueryParams.AddIgnoredActor(this);

    for (const FName& SocketName : SocketsToCheckForSight)
    {
        NumberOfLoSChecksPerformed++; // Keep track for Unreal's perception system

        FVector TargetLocation = GetMesh()->GetSocketLocation(SocketName);
        FHitResult HitResult;

        // Trace from the NPC's eyes to this specific body part
        bool bHitObstacle = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            ObserverLocation,
            TargetLocation,
            ECC_Visibility,
            QueryParams
        );

        // If nothing blocked the line, we found a visible point!
        if (!bHitObstacle)
        {
            OutSeenLocation = TargetLocation;
            OutSightStrength = 1.0f;
            return true;
        }
    }

    // If the loop finishes and we never returned true, nothing is visible
    OutSightStrength = 0.0f;
    return false;
}
void ABaseCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
    GetPerceptionLocationAndRotation(OutLocation, OutRotation);
}


// ai system
void ABaseCharacter::PostLoadInitialization_Implementation(const FStruct_NPCDataAssetPayload& LoadedPayload)
{
    // 1. Flush any existing behavior trees or stale goal states instantly
    if (ActionComponent)
    {
        ActionComponent->ForceResetComponentState();
    }

    // 2. Teleport Actor safely to its saved transform checkpoint boundaries
    SetActorLocationAndRotation(LoadedPayload.SavedLocation, LoadedPayload.SavedRotation, false, nullptr, ETeleportType::TeleportPhysics);

    // 3. Restore identity profile datasets
    NPCIdentityName = LoadedPayload.NPCIdentityName;
    InfiltrationProfile = LoadedPayload.InfiltrationProfile;

    // 4. Branch behavioral states completely away from standard engine possession timelines
    if (LoadedPayload.bIsDead)
    {
        InitializeCorpseState(LoadedPayload);
    }
    else
    {
        InitializeLivingState(LoadedPayload);
    }
}
FStruct_NPCDataAssetPayload ABaseCharacter::PackageSavePayload() const
{
    FStruct_NPCDataAssetPayload Payload;

    // Determine death status safely via the locomotion component state or controller tracking
    Payload.bIsDead = (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_None && ActionComponent == nullptr);

    Payload.SavedLocation = GetActorLocation();
    Payload.SavedRotation = GetActorRotation();
    Payload.NPCIdentityName = NPCIdentityName;
    Payload.InfiltrationProfile = InfiltrationProfile;

    // PLACEHOLDER LOOP: Query Narrative Equipment Component for active item string IDs to append here later
    // Payload.EquippedVisualTags = NarrativeEquipmentComponent->GetEquippedItemTags();

    return Payload;
}
void ABaseCharacter::InitializeCorpseState_Implementation(const FStruct_NPCDataAssetPayload& Payload)
{
    // 1. Permanently sever AI Controller connection to prevent phantom possession ticks
    if (AAIController* AIC = Cast<AAIController>(GetController()))
    {
        AIC->BrainComponent->StopLogic("Death Serialization Reset");
        AIC->UnPossess();
        AIC->Destroy();
    }

    // 2. Strip movement constraints and disable traditional capsule blocks
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 3. Configure the mesh for direct physics simulation
    GetMesh()->SetAllBodiesSimulatePhysics(false); // Reset tracking anchors
    GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // 4. Snap and simulate physics directly from the Pelvis bone structure to avoid terrain glitches
    GetMesh()->SetAllBodiesBelowSimulatePhysics(FName("pelvis"), true, true);

    // Blueprint hooks catch this block to safely load underwear meshes or cosmetic modifications!
}
void ABaseCharacter::InitializeLivingState_Implementation(const FStruct_NPCDataAssetPayload& Payload)
{
    // Ensure traditional collision boundaries are active
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetMesh()->SetAllBodiesSimulatePhysics(false);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    // Re-evaluate the action bidding matrix to execute optimal starting behavior sets instantly
    if (ActionComponent)
    {
        ActionComponent->ReevaluateStatePostLoad();
    }
}