#include "NPCSignificanceManager.h"
#include "AC_SignificanceComponent.h"
#include "SignificanceConfig.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"

// ── AUTONOMOUS ENGINE TICK ENTRY MATRIX ─────────────────────────────────────
void UNPCSignificanceManager::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    // Guard tracking environments: Ignore CDO templates and inactive worlds
    if (!World || HasAnyFlags(RF_ClassDefaultObject) || !World->IsGameWorld())
    {
        return;
    }
    if (GFrameNumber % 2 != 0)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    APlayerCameraManager* CamManager = PC->PlayerCameraManager;
    if (!CamManager) return;

    // Dynamically construct a viewpoint transform from the live player camera location
    FTransform CameraViewpoint;
    CameraViewpoint.SetLocation(CamManager->GetCameraLocation());
    CameraViewpoint.SetRotation(CamManager->GetCameraRotation().Quaternion());

    // Package it up into an array container view
    TArray<FTransform> ViewpointArray;
    ViewpointArray.Add(CameraViewpoint);

    // Fire the calculation loop natively
    Update(ViewpointArray);
}

bool UNPCSignificanceManager::IsTickable() const
{
    // Enable ticking only on active runtime operational instances
    return !HasAnyFlags(RF_ClassDefaultObject);
}

TStatId UNPCSignificanceManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UNPCSignificanceManager, STATGROUP_Tickables);
}
// ────────────────────────────────────────────────────────────────────────────

void UNPCSignificanceManager::Update(TArrayView<const FTransform> InViewpoints)
{
    Super::Update(InViewpoints);

    UWorld* World = GetWorld();
    if (!World) return;

    float DeltaSeconds = World->GetDeltaSeconds();

    APlayerController* PC = World->GetFirstPlayerController();
    CachedPlayerPawn = PC ? PC->GetPawn() : nullptr;
    CachedCameraManager = PC ? PC->PlayerCameraManager : nullptr;

    EvaluateBatch(DeltaSeconds);
    EvaluateCorpses();
}

void UNPCSignificanceManager::EvaluateBatch(float DeltaSeconds)
{
    if (RegisteredComponents.IsEmpty() || !CachedPlayerPawn) return;

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
            RegisteredComponents[Index]->EvaluateAndApply(CachedPlayerPawn, CachedCameraManager);
        }

        Evaluated++;

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
    // ── FIX: Match header array naming container 'Corpses' ───────────────────
    if (Corpses.IsEmpty()) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager) return;

    FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();

    // Persist our current evaluation placement index across engine frames
    static int32 CorpseBatchIndex = 0;

    int32 TotalCorpses = Corpses.Num();
    int32 EvaluationsThisFrame = FMath::Min(2, TotalCorpses);

    for (int32 i = 0; i < EvaluationsThisFrame; ++i)
    {
        // Enforce boundary safety constraints before reading memory entries
        CorpseBatchIndex = CorpseBatchIndex % TotalCorpses;

        if (IsValid(Corpses[CorpseBatchIndex]))
        {
            Corpses[CorpseBatchIndex]->EvaluateCorpsePureDistance(CameraLocation);
        }

        // Advance to the next tracking slot sequentially
        CorpseBatchIndex++;
    }
}

void UNPCSignificanceManager::RegisterNPC(UAC_SignificanceComponent* Component)
{
    if (Component && !RegisteredComponents.Contains(Component))
    {
        RegisteredComponents.Add(Component);
        UE_LOG(LogTemp, Warning, TEXT("NPCSignificanceManager: Registered %s | Total: %d"), *Component->GetOwner()->GetName(), RegisteredComponents.Num());
    }
}

void UNPCSignificanceManager::UnregisterNPC(UAC_SignificanceComponent* Component)
{
    if (Component)
    {
        RegisteredComponents.Remove(Component);
    }
}

void UNPCSignificanceManager::RegisterCorpse(UAC_SignificanceComponent* Component)
{
    // ── FIX: Corrected target registry to append straight to the Corpses array ──
    if (Component && !Corpses.Contains(Component))
    {
        Corpses.Add(Component);
        UE_LOG(LogTemp, Log, TEXT("NPCSignificanceManager: Shifting %s to background corpse tracking queue."), *Component->GetOwner()->GetName());
    }
}