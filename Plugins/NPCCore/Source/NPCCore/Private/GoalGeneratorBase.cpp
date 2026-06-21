#include "GoalGeneratorBase.h"
#include "ActivityComponent.h" // ◄ FIX: Compiler now fully sees the class layout!

UGoalItem* UGoalGeneratorBase::TryGenerateGoal_Implementation(AActor* Source, UActivityComponent* OwnerComp)
{
    if (!GeneratesGoalOfType || !OwnerComp) return nullptr;

    // The compiler now successfully casts OwnerComp to UObject*
    UGoalItem* NewGoal = NewObject<UGoalItem>(OwnerComp, GeneratesGoalOfType);

    if (NewGoal)
    {
        NewGoal->SourceActor = Source;
        NewGoal->Initialize(OwnerComp);
    }

    return NewGoal;
}