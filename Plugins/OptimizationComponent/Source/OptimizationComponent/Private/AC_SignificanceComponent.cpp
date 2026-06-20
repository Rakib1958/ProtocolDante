#include "AC_SignificanceComponent.h"
#include "Components/SkeletalMeshComponent.h" 
#include "SkeletalMeshComponentBudgeted.h"  
#include "AC_NPC_Clothing.h"
#include "MMAnimInstance.h"
#include "IAnimationBudgetAllocator.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "NPCSignificanceManager.h"

UAC_SignificanceComponent::UAC_SignificanceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAC_SignificanceComponent::Initialize(ENPCType InNPCType)
{
    NPCType = InNPCType;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    TArray<UActorComponent*> Found = Owner->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName("Body"));

    if (!Found.IsEmpty())
    {
        BodyMesh = Cast<USkeletalMeshComponent>(Found[0]);
        // The type is now fully defined, allowing Cast to execute cleanly
        BudgetedBodyMesh = Cast<USkeletalMeshComponentBudgeted>(Found[0]);
    }

    ClothingComp = Owner->FindComponentByClass<UAC_NPC_Clothing>();

    AnimInstance = BodyMesh ? Cast<UMMAnimInstance>(BodyMesh->GetAnimInstance()) : nullptr;

    BudgetAllocator = IAnimationBudgetAllocator::Get(GetWorld());

    if (UWorld* World = GetWorld())
    {
        USignificanceManager* RawManager = USignificanceManager::Get(World);
        UNPCSignificanceManager* Manager = Cast<UNPCSignificanceManager>(RawManager);

        if (Manager)
        {
            Manager->RegisterNPC(this);
            bInitialized = true;
        }
        else
        {
            // The editor will now load safely and output this red warning text instead of crashing!
            UE_LOG(LogTemp, Error, TEXT("UAC_SignificanceComponent: Active Significance Manager is not UNPCSignificanceManager! Check your Project Settings or DefaultEngine.ini."));
            bInitialized = false;
        }
    }
    UE_LOG(LogTemp, Warning,
        TEXT("AnimInstance valid: %s"),
        AnimInstance ? TEXT("YES") : TEXT("NO"));
}

void UAC_SignificanceComponent::SetAlertState(EAlertState NewState)
{
    CurrentAlertState = NewState;
}

void UAC_SignificanceComponent::TriggerOverride(float Duration)
{
    float UseDuration = Duration > 0.f ? Duration : (Config ? Config->EventOverrideDuration : 5.f);
    OverrideTimeRemaining = FMath::Max(OverrideTimeRemaining, UseDuration);
}

void UAC_SignificanceComponent::OnOwnerDied()
{
    bIsDead = true;
    OverrideTimeRemaining = 0.f;

    if (BudgetAllocator && BudgetedBodyMesh)
    {
        BudgetAllocator->SetComponentSignificance(BudgetedBodyMesh, 0.f);
    }

    if (UWorld* World = GetWorld())
    {
        if (UNPCSignificanceManager* Manager = Cast<UNPCSignificanceManager>(USignificanceManager::Get(World)))
        {
            Manager->UnregisterNPC(this);
            Manager->RegisterCorpse(this);
        }
    }
}

void UAC_SignificanceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (OverrideTimeRemaining > 0.f)
        OverrideTimeRemaining -= DeltaTime;
}

float UAC_SignificanceComponent::CalculatePerceivedDistance(APawn* PlayerPawn, APlayerCameraManager* CameraManager) const
{
    if (!PlayerPawn || !CameraManager || !GetOwner()) return TNumericLimits<float>::Max();

    float WorldDistance = FVector::Dist(GetOwner()->GetActorLocation(), CameraManager->GetCameraLocation());

    float FOV = CameraManager->GetFOVAngle();
    float MappedFOV = FMath::GetMappedRangeValueClamped(FVector2D(60.f, 120.f), FVector2D(1.f, 0.f), FOV);

    return WorldDistance * FMath::Max(MappedFOV, 0.1f);
}

float UAC_SignificanceComponent::CalculateSignificance(APawn* PlayerPawn, APlayerCameraManager* CameraManager) const
{
    if (!Config || !GetOwner()) return 0.f;

    float WorldDistance = PlayerPawn ? FVector::Dist(GetOwner()->GetActorLocation(), PlayerPawn->GetActorLocation()) : TNumericLimits<float>::Max();

    if (WorldDistance <= Config->VisibilityCheckMaxDistance)
    {
        if (BodyMesh && BodyMesh->WasRecentlyRendered(0.1f))
            return 1.f;
    }

    if (HasActiveOverride())
        return 1.f;

    float PerceivedDistance = CalculatePerceivedDistance(PlayerPawn, CameraManager);
    float MaxDist = Config->Tiers.IsEmpty() ? 5000.f : Config->Tiers.Last().MaxDistance;

    float DistanceFactor = FMath::Clamp(1.f - (PerceivedDistance / MaxDist), 0.f, 1.f);

    float AlertFactor = 0.f;
    switch (CurrentAlertState)
    {
    case EAlertState::Unaware:    AlertFactor = 0.0f; break;
    case EAlertState::Suspicious: AlertFactor = 0.3f; break;
    case EAlertState::Alerted:    AlertFactor = 0.6f; break;
    case EAlertState::Searching:  AlertFactor = 0.7f; break;
    case EAlertState::Engaging:   AlertFactor = 1.0f; break;
    }

    float TypeFactor = 0.f;
    if (const int32* BaseTier = Config->BaseSignificanceTier.Find(NPCType))
    {
        int32 MaxTier = FMath::Max(Config->Tiers.Num() - 1, 1);
        TypeFactor = 1.f - ((float)*BaseTier / (float)MaxTier);
    }

    return FMath::Clamp(
        (DistanceFactor * Config->DistanceWeight) +
        (AlertFactor * Config->AlertWeight) +
        (TypeFactor * Config->TypeWeight),
        0.f, 1.f);
}

int32 UAC_SignificanceComponent::ResolveToTier(float Significance) const
{
    if (!Config || Config->Tiers.IsEmpty()) return 0;

    int32 NumTiers = Config->Tiers.Num();
    float TierStep = 1.f / (float)NumTiers;

    for (int32 i = 0; i < NumTiers - 1; i++)
    {
        if (Significance >= 1.f - (TierStep * (i + 1)))
            return i;
    }
    return NumTiers - 1;
}

void UAC_SignificanceComponent::ApplyTier(int32 NewTier)
{
    if (!Config || !Config->Tiers.IsValidIndex(NewTier)) return;
    if (NewTier == CurrentTier) return;

    int32 OldTier = CurrentTier;
    CurrentTier = NewTier;

    const FSignificanceTierConfig& TierConfig = Config->Tiers[NewTier];

    if (ClothingComp)
    {
        ClothingComp->SetMeshLODBias(TierConfig.ForcedLOD);
        ClothingComp->SetMeshVisibility(!TierConfig.bCullMeshes);
        ClothingComp->SetFollowerTickInterval(TierConfig.FollowerTickInterval);
    }

    if (BudgetAllocator && BudgetedBodyMesh)
        BudgetAllocator->SetComponentSignificance(
            BudgetedBodyMesh, TierConfig.BudgetAllocatorSignificance);

    // Re-cache AnimInstance if it was null at Initialize time
    if (!AnimInstance && BodyMesh)
        AnimInstance = Cast<UMMAnimInstance>(BodyMesh->GetAnimInstance());

    //if (AnimInstance)
    //    AnimInstance->CurrentSignificanceTier = NewTier;
    if (AnimInstance)
    {
        // Search the runtime class metadata for the exact variable identifier name
        static FName TierVarName(TEXT("CurrentSignificanceTier"));
        if (FIntProperty* Prop = FindFProperty<FIntProperty>(AnimInstance->GetClass(), TierVarName))
        {
            // Safely write the memory value straight into the instance instance space
            Prop->SetValue_InContainer(AnimInstance, NewTier);
        }
    }

    OnTierChanged.Broadcast(OldTier, NewTier);
}

void UAC_SignificanceComponent::EvaluateAndApply(APawn* PlayerPawn, APlayerCameraManager* CameraManager)
{
    if (!bInitialized || bIsDead || !GetOwner()) return;

    float Significance = CalculateSignificance(PlayerPawn, CameraManager);
    int32 NewTier = ResolveToTier(Significance);

    // ── NATIVE VIEWPORT DEBUG TEXT OVERLAY ──────────────────────────────────────
    if (GEngine)
    {
        // Color-code the text dynamically based on the target performance tier
        FColor TierColor = FColor::White;
        FString TierName = TEXT("UNKNOWN");

        switch (NewTier)
        {
        case 0: TierColor = FColor::Cyan;    TierName = TEXT("TIER 0 (High / Full Smooth)"); break;
        case 1: TierColor = FColor::Green;   TierName = TEXT("TIER 1 (Normal / Throttling)"); break;
        case 2: TierColor = FColor::Yellow;  TierName = TEXT("TIER 2 (Low / Budgeted)"); break;
        case 3: TierColor = FColor::Red;     TierName = TEXT("TIER 3 (Culled / Asleep)"); break;
        }

        // Format a comprehensive single-line diagnostic readout
        FString DebugMsg = FString::Printf(TEXT("%s ── Score: %.2f | Active: %s | Override: %s"),
            *GetOwner()->GetName(),
            Significance,
            *TierName,
            HasActiveOverride() ? TEXT("TRUE") : TEXT("FALSE"));

        // Use the actor's unique runtime ID as the key channel. 
        // This ensures each guard refreshes their own single row instead of flooding the screen vertically!
        GEngine->AddOnScreenDebugMessage(
            static_cast<uint64>(GetOwner()->GetUniqueID()),
            1.5f,        // Display duration (slightly longer than batch tick rate to avoid flickering)
            TierColor,   // Color map
            DebugMsg     // Parsed message string
        );
    }

    ApplyTier(NewTier);
}

USkeletalMeshComponent* UAC_SignificanceComponent::FindMeshByTag(FName Tag) const
{
    if (!GetOwner()) return nullptr;
    TArray<UActorComponent*> Found = GetOwner()->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), Tag);
    return Found.IsEmpty() ? nullptr : Cast<USkeletalMeshComponent>(Found[0]);
}