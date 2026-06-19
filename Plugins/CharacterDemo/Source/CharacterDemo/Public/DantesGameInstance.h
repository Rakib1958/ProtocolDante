// NPCGameInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DantesGameInstance.generated.h"

UCLASS()
class CHARACTERDEMO_API UDantesGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // ── Chaos Level ──────────────────────────────────────────────────────

    // Persistent across story, saved via Easy Game UI on BP side
    UPROPERTY(BlueprintReadWrite, Category = "Chaos")
    float PersistentChaosLevel = 0.f;

    UFUNCTION(BlueprintCallable, Category = "Chaos")
    void AddChaos(float Amount)
    {
        PersistentChaosLevel = FMath::Clamp(PersistentChaosLevel + Amount, 0.f, 100.f);
    }

    UFUNCTION(BlueprintCallable, Category = "Chaos")
    void ReduceChaosOnLevelComplete(float Amount = 20.f)
    {
        PersistentChaosLevel = FMath::Clamp(PersistentChaosLevel - Amount, 0.f, 100.f);
    }

    UFUNCTION(BlueprintPure, Category = "Chaos")
    float GetPersistentChaosLevel() const { return PersistentChaosLevel; }

    UFUNCTION(BlueprintPure, Category = "Chaos")
    bool ShouldSpawnMilitary() const { return PersistentChaosLevel >= 50.f; }

    // ── Session chaos (discarded on reload, never saved) ─────────────────

    UPROPERTY(BlueprintReadWrite, Category = "Chaos")
    float SessionChaosLevel = 0.f;

    UFUNCTION(BlueprintCallable, Category = "Chaos")
    void ResetSessionChaos() { SessionChaosLevel = 0.f; }

    UFUNCTION(BlueprintCallable, Category = "Chaos")
    void AddSessionChaos(float Amount)
    {
        SessionChaosLevel = FMath::Clamp(SessionChaosLevel + Amount, 0.f, 100.f);
    }

    // Effective chaos = persistent only (session is runtime-only)
    UFUNCTION(BlueprintPure, Category = "Chaos")
    float GetEffectiveChaosLevel() const { return PersistentChaosLevel; }
};