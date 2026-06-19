#include "NPCSignificanceManager.h"
#include "AC_SignificanceComponent.h"
#include "SignificanceConfig.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

// FIX: Completely removed the invalid BeginPlay block. Caching now happens dynamically per-frame.

void UNPCSignificanceManager::Update(TArrayView<const FTransform> Viewpoints)
{
    // FIX: Pass the array view context straight back up to the engine parent
    Super::Update(Viewpoints);

    UWorld* World = GetWorld();
    if (!World) return;

    // Extract frame delta times natively from the world context
    float DeltaSeconds = World->GetDeltaSeconds();

    // Safe once-per-frame lazy caching assignment
    APlayerController* PC = World->GetFirstPlayerController();
    CachedPlayerPawn = PC ? PC->GetPawn() : nullptr;
    CachedCameraManager = PC ? PC->PlayerCameraManager : nullptr;

    EvaluateBatch(DeltaSeconds);
    EvaluateCorpses();
}

void UNPCSignificanceManager::EvaluateBatch(float DeltaSeconds)
{
    // FIX: Replaced out-of-bounds 'ActiveNPCs' calls with 'RegisteredComponents'
    if (RegisteredComponents.IsEmpty()) return;
    if (!CachedPlayerPawn) return;

    float BudgetMs = 0.1f;
    if (RegisteredComponents[0] && RegisteredComponents[0]->Config)
        BudgetMs = RegisteredComponents[0]->Config->EvaluationBudgetMs;

    const double BudgetSeconds = BudgetMs * 0.001;
    const double StartTime = FPlatformTime::Seconds();
    const int32 Total = RegisteredComponents.Num();
    int32 Evaluated = 0;

    while (Evaluated < Total)
    {
        int32 Index = (CurrentBatchIndex + Evaluated) % Total;

        if (IsValid(RegisteredComponents[Index]))
        {
            // Triggers the processing pass
            RegisteredComponents[Index]->EvaluateAndApply(CachedPlayerPawn, CachedCameraManager);
        }

        Evaluated++;

        // Staggered budget timeout exit gate
        if ((FPlatformTime::Seconds() - StartTime) >= BudgetSeconds)
        {
            CurrentBatchIndex = (CurrentBatchIndex + Evaluated) % Total;
            return;
        }
    }

    CurrentBatchIndex = 0;
}

void UNPCSignificanceManager::EvaluateCorpses()
{
    // Stub implementation: Put dead actor pooling/culling routines here if needed
}

float UNPCSignificanceManager::CalculateSignificance(UAC_SignificanceComponent* Comp) const
{
    UWorld* World = GetWorld();
    if (!Comp || !World || !Config) return 0.f;

    AActor* Owner = Comp->GetOwner();
    if (!Owner) return 0.f;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return 0.f;

    // ── Visibility check (highest precedence) ──────────────────────────

    APawn* PlayerPawn = PC->GetPawn();
    float DistanceToPlayer = PlayerPawn
        ? FVector::Dist(Owner->GetActorLocation(), PlayerPawn->GetActorLocation())
        : TNumericLimits<float>::Max();

    if (DistanceToPlayer <= Config->VisibilityCheckMaxDistance)
    {
        // FIX: Changed GetLeaderMesh() to GetBodyMesh() to match your component API
        USkeletalMeshComponent* BodyMesh = Comp->GetBodyMesh();
        if (BodyMesh && BodyMesh->WasRecentlyRendered(0.1f))
        {
            return 1.0f; // Rendered -> instantly evaluate at maximum fidelity
        }
    }

    // ── Override channel (events, alarms, gunshots) ─────────────────────

    if (Comp->HasActiveOverride())
    {
        return 1.0f;
    }

    // ── Distance factor ──────────────────────────────────────────────────

    float MaxDist = Config->Tiers.Num() > 0 ? Config->Tiers.Last().MaxDistance : 10000.f;
    float DistanceFactor = FMath::Clamp(1.f - (DistanceToPlayer / MaxDist), 0.f, 1.f);

    // ── Alert state factor ───────────────────────────────────────────────

    float AlertFactor = 0.f;
    switch (Comp->GetAlertState())
    {
    case EAlertState::Unaware:    AlertFactor = 0.0f; break;
    case EAlertState::Suspicious: AlertFactor = 0.3f; break;
    case EAlertState::Alerted:    AlertFactor = 0.6f; break;
    case EAlertState::Searching:  AlertFactor = 0.7f; break;
    case EAlertState::Engaging:   AlertFactor = 1.0f; break;
    }

    // ── NPC type base factor ─────────────────────────────────────────────

    float TypeFactor = 0.f;
    if (const int32* BaseTier = Config->BaseSignificanceTier.Find(Comp->GetNPCType()))
    {
        int32 MaxTier = Config->Tiers.Num() - 1;
        TypeFactor = MaxTier > 0 ? 1.f - ((float)*BaseTier / (float)MaxTier) : 1.f;
    }

    // ── Weighted sum ─────────────────────────────────────────────────────

    float Significance = (DistanceFactor * Config->DistanceWeight)
        + (AlertFactor * Config->AlertWeight)
        + (TypeFactor * Config->TypeWeight);

    return FMath::Clamp(Significance, 0.f, 1.f);
}

int32 UNPCSignificanceManager::ResolveToTier(UAC_SignificanceComponent* Comp, float Significance) const
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

void UNPCSignificanceManager::RegisterNPC(UAC_SignificanceComponent* Component)
{
    if (Component && !RegisteredComponents.Contains(Component))
    {
        RegisteredComponents.Add(Component);
    }
}

void UNPCSignificanceManager::UnregisterNPC(UAC_SignificanceComponent* Component)
{
    if (Component)
    {
        RegisteredComponents.Remove(Component);
    }
}