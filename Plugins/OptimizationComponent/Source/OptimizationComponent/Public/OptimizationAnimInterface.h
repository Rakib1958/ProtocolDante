// Inside OptimizationComponent -> Public -> OptimizationAnimInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OptimizationAnimInterface.generated.h"

UINTERFACE(Blueprintable)
class UOptimizationAnimInterface : public UInterface
{
    GENERATED_BODY()
};

class OPTIMIZATIONCOMPONENT_API IOptimizationAnimInterface
{
    GENERATED_BODY()

public:
    // BlueprintNativeEvent allows this to be cleanly implemented straight inside ABP_NPC_Base
    UFUNCTION(BlueprintNativeEvent, Category = "Optimization | Animation")
    void SetSignificanceTier(int32 NewTier);
};