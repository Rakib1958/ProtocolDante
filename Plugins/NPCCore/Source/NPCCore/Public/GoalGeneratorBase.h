#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GoalItem.h"
#include "GoalGeneratorBase.generated.h"

class UActivityComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew)
class NPCCORE_API UGoalGeneratorBase : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generator")
    TSubclassOf<UGoalItem> GeneratesGoalOfType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generator")
    bool bFireOnInitialize = false;

    UFUNCTION(BlueprintNativeEvent, Category = "Generator")
    UGoalItem* TryGenerateGoal(AActor* Source, UActivityComponent* OwnerComp);

    // Declaration only: Implementation moved to .cpp
    virtual UGoalItem* TryGenerateGoal_Implementation(AActor* Source, UActivityComponent* OwnerComp);
};