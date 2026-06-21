// AAIC_NPCController.h
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AAIC_NPCController.generated.h"

UCLASS()
class NPCCORE_API AAIC_NPCController : public AAIController
{
    GENERATED_BODY()

public:
    AAIC_NPCController();

    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception")
    //TObjectPtr<UAIPerceptionComponent> AIPerception;
};