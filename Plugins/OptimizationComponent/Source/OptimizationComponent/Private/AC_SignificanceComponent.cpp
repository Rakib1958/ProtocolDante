#include "AC_SignificanceComponent.h"
#include "OptimizationAnimInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "IAnimationBudgetAllocator.h"
#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "NPCSignificanceManager.h"

UAC_SignificanceComponent::UAC_SignificanceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAC_SignificanceComponent::InitializeComponentCustom()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    TArray<UActorComponent*> Found = Owner->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName("Body"));
    if (!Found.IsEmpty())
    {
        BodyMesh = Cast<USkeletalMeshComponent>(Found[0]);
        BudgetedBodyMesh = Cast<USkeletalMeshComponentBudgeted>(Found[0]);
    }

    // ── FIX 1: REFLECTION DISCOVERY MATRIX ──────────────────────────────────
    // Locate and cache the decoupled clothing component handle automatically
    static FName FunctionContractName(TEXT("ApplyClothingLODBias"));
    for (UActorComponent* Comp : Owner->GetComponents())
    {
        if (Comp && Comp->FindFunction(FunctionContractName))
        {
            CachedMessagingTarget = Comp;
            break;
        }
    }

    BudgetAllocator = IAnimationBudgetAllocator::Get(GetWorld());

    if (UWorld* World = GetWorld())
    {
        if (UNPCSignificanceManager* Manager = Cast<UNPCSignificanceManager>(USignificanceManager::Get(World)))
        {
            Manager->RegisterNPC(this);
            bInitialized = true;
        }
    }
}

float UAC_SignificanceComponent::CalculateSignificance(APawn* PlayerPawn, APlayerCameraManager* CameraManager) const
{
    if (!Config || Config->Tiers.IsEmpty() || !GetOwner() || bIsDead) return 0.f;

    float WorldDistance = PlayerPawn ? FVector::Dist(GetOwner()->GetActorLocation(), PlayerPawn->GetActorLocation()) : TNumericLimits<float>::Max();

    // 1. Fully-Configured Smooth Visibility Falloff Matrix

    if (HasActiveOverride()) return 1.f;
    if (BodyMesh && BodyMesh->WasRecentlyRendered(0.1f))
    {
        int32 NumTiers = Config->Tiers.Num();
        float TierStep = 1.f / (float)NumTiers;

        for (int32 i = 0; i < NumTiers; i++)
        {
            float MinDistance = (i == 0) ? 0.f : Config->Tiers[i - 1].MaxDistance;
            float MaxDistance = Config->Tiers[i].MaxDistance;

            if (WorldDistance >= MinDistance && WorldDistance <= MaxDistance)
            {
                float DistanceDelta = MaxDistance - MinDistance;
                float Alpha = (DistanceDelta > 0.f) ? ((WorldDistance - MinDistance) / DistanceDelta) : 0.f;

                float MaxSignificanceForTier = 1.f - (TierStep * i);
                float MinSignificanceForTier = 1.f - (TierStep * (i + 1));

                return FMath::Lerp(MaxSignificanceForTier, MinSignificanceForTier, Alpha);
            }
        }

        if (WorldDistance > Config->Tiers.Last().MaxDistance) return 0.f;
    }
    //FString DebugMsg = FString::Printf(TEXT("%s ── Score: %.2f | Active Tier: %d"), *GetOwner()->GetName(), Significance, NewTier);
    //GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetOwner()->GetUniqueID()), 1.5f, FColor::Red, FString::Printf(TEXT("Running")));
    

    // 2. Fallback Background Calculation Matrix
    float PerceivedDistance = CalculatePerceivedDistance(PlayerPawn, CameraManager);
    float MaxDist = Config->Tiers.Last().MaxDistance;
    float DistanceFactor = FMath::Clamp(1.f - (PerceivedDistance / MaxDist), 0.f, 1.f);

    float TypeFactor = 0.f;
    if (const int32* BaseTier = Config->BaseSignificanceTier.Find(NPCTypeTag))
    {
        int32 MaxTier = FMath::Max(Config->Tiers.Num() - 1, 1);
        TypeFactor = 1.f - ((float)*BaseTier / (float)MaxTier);
    }

    return FMath::Clamp((DistanceFactor * Config->DistanceWeight) + (TypeFactor * Config->TypeWeight), 0.f, 1.f);
}

int32 UAC_SignificanceComponent::ResolveToTier(float Significance) const
{
    if (!Config || Config->Tiers.IsEmpty()) return 2;

    int32 NumTiers = Config->Tiers.Num();
    float TierStep = 1.f / (float)NumTiers;

    for (int32 i = 0; i < NumTiers - 1; i++)
    {
        if (Significance >= 1.f - (TierStep * (i + 1)))
            return i;
    }
    return NumTiers - 1;
}

void UAC_SignificanceComponent::EvaluateAndApply(APawn* PlayerPawn, APlayerCameraManager* CameraManager)
{
    float DeltaTime = GetWorld()->GetDeltaSeconds();
    if (OverrideTimeRemaining > 0.f)
        OverrideTimeRemaining -= DeltaTime;
    if (!bInitialized || bIsDead) return;

    float Significance = CalculateSignificance(PlayerPawn, CameraManager);
    int32 NewTier = ResolveToTier(Significance);

    // Dynamic viewport debugging readout row allocation pass
    if (GEngine)
    {
        FColor TierColor = FColor::White;
        switch (NewTier)
        {
        case 0: TierColor = FColor::Cyan; break;
        case 1: TierColor = FColor::Green; break;
        case 2: TierColor = FColor::Yellow; break;
        case 3: TierColor = FColor::Red; break;
        }

        FString DebugMsg = FString::Printf(TEXT("%s ── Score: %.2f | Active Tier: %d"), *GetOwner()->GetName(), Significance, NewTier);
        if (bShouldDebug) GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetOwner()->GetUniqueID()), 1.5f, TierColor, DebugMsg);
    }

    ApplyTier(NewTier);
}

void UAC_SignificanceComponent::ApplyTier(int32 NewTier)
{
    if (!Config || !Config->Tiers.IsValidIndex(NewTier)) return;
    if (NewTier == CurrentTier) return;

    int32 OldTier = CurrentTier;
    CurrentTier = NewTier;

    const FSignificanceTierConfig& TierConfig = Config->Tiers[NewTier];

    // ── REFLECTION RUNTIME EXECUTOR PASS ─────────────────────────────────────
    if (CachedMessagingTarget.IsValid())
    {
        UActorComponent* TargetComp = CachedMessagingTarget.Get();

        // 1. Invoke LOD Bias
        if (UFunction* LODFunc = TargetComp->FindFunction(TEXT("ApplyClothingLODBias")))
        {
            int32 Param = TierConfig.ForcedLOD;
            TargetComp->ProcessEvent(LODFunc, &Param);
        }

        // 2. Invoke Visibility
        if (UFunction* VisFunc = TargetComp->FindFunction(TEXT("ApplyClothingVisibility")))
        {
            bool Param = !TierConfig.bCullMeshes;
            TargetComp->ProcessEvent(VisFunc, &Param);
        }

        // 3. Invoke Tick Interval
        if (UFunction* TickFunc = TargetComp->FindFunction(TEXT("ApplyClothingFollowerTickInterval")))
        {
            float Param = TierConfig.FollowerTickInterval;
            TargetComp->ProcessEvent(TickFunc, &Param);
        }
    }

    // Budget allocator significance
    if (BudgetAllocator && BudgetedBodyMesh)
        BudgetAllocator->SetComponentSignificance(
            BudgetedBodyMesh, TierConfig.BudgetAllocatorSignificance);

    // Notify Blueprint (clothing LOD, locomotion tick etc)
    OnSignificanceTierChanged.Broadcast(OldTier, NewTier);

    // Notify AnimInstance via interface
    if (BodyMesh)
    {
        UAnimInstance* AnimInst = BodyMesh->GetAnimInstance();
        if (AnimInst && AnimInst->GetClass()->ImplementsInterface(
            UOptimizationAnimInterface::StaticClass()))
        {
            //IOptimizationAnimInterface::Execute_SetSignificanceTier(
            //    AnimInst, NewTier);
            
        }
        static FName TierVarName(TEXT("CurrentSignificanceTier"));
        if (FIntProperty* Prop = FindFProperty<FIntProperty>(AnimInst->GetClass(), TierVarName))
        {
            Prop->SetValue_InContainer(AnimInst, NewTier);
        }
    }

}

float UAC_SignificanceComponent::CalculatePerceivedDistance(APawn* PlayerPawn, APlayerCameraManager* CameraManager) const
{
    if (!PlayerPawn) return TNumericLimits<float>::Max();

    float WorldDistance = FVector::Dist(
        GetOwner()->GetActorLocation(),
        PlayerPawn->GetActorLocation());

    if (!CameraManager) return WorldDistance;

    float FOV = CameraManager->GetFOVAngle();
    float MappedFOV = FMath::GetMappedRangeValueClamped(
        FVector2D(60.f, 120.f),
        FVector2D(1.f, 0.f),
        FOV);

    return WorldDistance * FMath::Max(MappedFOV, 0.1f);
}

void UAC_SignificanceComponent::TriggerOverride(float Duration)
{
    float UseDuration = Duration > 0.f
        ? Duration
        : (Config ? Config->EventOverrideDuration : 5.f);
    OverrideTimeRemaining = FMath::Max(OverrideTimeRemaining, UseDuration);
}

void UAC_SignificanceComponent::OnOwnerDied()
{
    if (bIsDead) return;
    bIsDead = true;
    OverrideTimeRemaining = 0.f;

    if (BudgetAllocator && BudgetedBodyMesh)
        BudgetAllocator->SetComponentSignificance(BudgetedBodyMesh, 0.f);

    // ── FIX 2: CORPSE VISUAL LOCKDOWN ────────────────────────────────────────
    // Explicitly command the clothing component to lock into a permanent, non-dynamic state.
    // Forcing an explicit LOD bias completely strips out engine FOV screen-size calculations!
    if (CachedMessagingTarget.IsValid())
    {
        UActorComponent* TargetComp = CachedMessagingTarget.Get();

        // 1. Force absolute high-quality LOD 0 (or LOD 1 for corpses) to lock out dynamic pops
        if (UFunction* LODFunc = TargetComp->FindFunction(TEXT("ApplyClothingLODBias")))
        {
            int32 CorpseForcedLOD = 0;
            TargetComp->ProcessEvent(LODFunc, &CorpseForcedLOD);
        }

        // 2. Ensure visibility is clamped to TRUE permanently
        if (UFunction* VisFunc = TargetComp->FindFunction(TEXT("ApplyClothingVisibility")))
        {
            bool bVisible = true;
            TargetComp->ProcessEvent(VisFunc, &bVisible);
        }

        // 3. Relax tick optimizations since they are no longer moving
        if (UFunction* TickFunc = TargetComp->FindFunction(TEXT("ApplyClothingFollowerTickInterval")))
        {
            float DeadTickInterval = 0.0f; // Default baseline engine ticking for static poses
            TargetComp->ProcessEvent(TickFunc, &DeadTickInterval);
        }
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

void UAC_SignificanceComponent::EvaluateCorpsePureDistance(const FVector& CameraLocation)
{
    if (!GetOwner()) return;

    // 1. Calculate absolute, raw world distance (Zero FOV weight!)
    float DistanceToCamera = FVector::Dist(GetOwner()->GetActorLocation(), CameraLocation);

    // 2. Map distance thresholds directly to explicit forced LOD layers
    int32 IntendedLOD = 0;
    bool bShouldBeVisible = true;

    if (DistanceToCamera > 5000.f) // Beyond 50 meters, cull the clothes completely
    {
        bShouldBeVisible = false;
    }
    else if (DistanceToCamera > 2500.f) // Medium-Far distance
    {
        IntendedLOD = 2;
    }
    else if (DistanceToCamera > 1000.f) // Medium distance
    {
        IntendedLOD = 1;
    }
    else // Close proximity
    {
        IntendedLOD = 0;
    }

    // 3. Force the clothing component to use this exact LOD layer.
    // Setting an explicit ForcedLOD explicitly overrides and shuts down Unreal's auto-LOD FOV calculation!
    if (CachedMessagingTarget.IsValid())
    {
        UActorComponent* TargetComp = CachedMessagingTarget.Get();

        if (UFunction* LODFunc = TargetComp->FindFunction(TEXT("ApplyClothingLODBias")))
        {
            TargetComp->ProcessEvent(LODFunc, &IntendedLOD);
        }

        if (UFunction* VisFunc = TargetComp->FindFunction(TEXT("ApplyClothingVisibility")))
        {
            TargetComp->ProcessEvent(VisFunc, &bShouldBeVisible);
        }
    }
}