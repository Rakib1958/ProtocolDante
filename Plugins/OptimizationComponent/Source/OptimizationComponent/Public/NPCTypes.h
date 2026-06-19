// NPCTypes.h
#pragma once

#include "CoreMinimal.h"
#include "NPCTypes.generated.h"

UENUM(BlueprintType)
enum class ENPCType : uint8
{
    Worker,
    Warden,
    Agent,
    Military,
    Civilian
};

UENUM(BlueprintType)
enum class EAlertState : uint8
{
    Unaware,
    Suspicious,
    Alerted,
    Searching,
    Engaging
};

UENUM(BlueprintType)
enum class EGoalPriority : uint8
{
    Background = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

UENUM(BlueprintType)
enum class EGoalRemoveReason : uint8
{
    Succeeded,
    Expired,
    Interrupted,
    OwnerDied
};