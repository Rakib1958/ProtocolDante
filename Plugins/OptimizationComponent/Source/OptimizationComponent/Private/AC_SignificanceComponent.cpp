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

    BudgetAllocator = IAnimationBudgetAllocator::Get(GetWorld());
    //if (BudgetAllocator && BudgetedBodyMesh)
    //{
    //    BudgetAllocator->RegisterComponent(BudgetedBodyMesh);
    //}

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
            static FName TierVarName(TEXT("CurrentSignificanceTier"));
            if (FIntProperty* Prop = FindFProperty<FIntProperty>(AnimInst->GetClass(), TierVarName))
            {
                Prop->SetValue_InContainer(AnimInst, NewTier);
            }
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
    bIsDead = true;
    OverrideTimeRemaining = 0.f;

    if (BudgetAllocator && BudgetedBodyMesh)
        BudgetAllocator->SetComponentSignificance(BudgetedBodyMesh, 0.f);

    if (UWorld* World = GetWorld())
    {
        if (UNPCSignificanceManager* Manager =
            Cast<UNPCSignificanceManager>(USignificanceManager::Get(World)))
        {
            Manager->UnregisterNPC(this);
            Manager->RegisterCorpse(this);
        }
    }
}