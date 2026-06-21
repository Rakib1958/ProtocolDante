// ActivityComponent.cpp
#include "ActivityComponent.h"
#include "AAIC_NPCController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UActivityComponent::UActivityComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
}

void UActivityComponent::Initialize()
{
    if (!NPCDefinition) return;

    // Fire all generators marked bFireOnInitialize
    // Source = nullptr since no perception event triggered this
    for (UGoalGeneratorBase* Gen : NPCDefinition->GoalGenerators)
    {
        if (!Gen || !Gen->bFireOnInitialize) continue;

        UGoalItem* Goal = Gen->TryGenerateGoal(nullptr, this);
        if (Goal)
            GoalList.Add(Goal);
    }

    // Initial rescore to pick starting activity
    bNeedsImmediateRescore = true;
}

void UActivityComponent::TickComponent(float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsDead || !NPCDefinition || !bRescoringEnabled) return;

    CleanupExpiredGoals(DeltaTime);

    if (bNeedsImmediateRescore)
    {
        bNeedsImmediateRescore = false;
        RescoringTimer = 0.f;
        Rescore();
    }
    else
    {
        RescoringTimer += DeltaTime;
        if (RescoringTimer >= RescoringInterval)
        {
            RescoringTimer = 0.f;
            Rescore();
        }
    }
}

void UActivityComponent::AddGoal(
    TSubclassOf<UGoalItem> GoalClass, AActor* Source)
{
    if (!GoalClass) return;

    UGoalItem* Existing = FindExistingGoal(GoalClass);

    if (Existing)
    {
        // Create candidate to compare scores
        UGoalItem* Candidate = NewObject<UGoalItem>(this, GoalClass);
        Candidate->SourceActor = Source;

        if (Candidate->GetGoalScore() > Existing->GetGoalScore())
        {
            // Update existing — reset lifetime, update source
            Existing->SourceActor = Source;
            Existing->ResetLifetime();
            Existing->Initialize(this);

            // Only trigger immediate rescore if meaningful priority
            if (Existing->Priority >= EGoalPriority::Normal)
                bNeedsImmediateRescore = true;
        }
        return;
    }

    // Fresh goal
    UGoalItem* NewGoal = NewObject<UGoalItem>(this, GoalClass);
    if (!NewGoal) return;

    NewGoal->SourceActor = Source;
    NewGoal->Initialize(this);
    GoalList.Add(NewGoal);

    // Background goals don't interrupt — everything else does
    if (NewGoal->Priority > EGoalPriority::Background)
        bNeedsImmediateRescore = true;
}

void UActivityComponent::RemoveGoal(
    UGoalItem* Goal, EGoalRemoveReason Reason)
{
    if (!Goal) return;
    Goal->OnRemoved(Reason);
    GoalList.Remove(Goal);
}

void UActivityComponent::ClearAllGoals(EGoalRemoveReason Reason)
{
    for (UGoalItem* Goal : GoalList)
        if (Goal) Goal->OnRemoved(Reason);
    GoalList.Empty();
}

void UActivityComponent::SetAlertState(EAlertState NewState)
{
    if (CurrentAlertState == NewState) return;
    CurrentAlertState = NewState;
    OnAlertStateChanged.Broadcast(NewState);
    // Alert state change warrants a rescore
    bNeedsImmediateRescore = true;
}

void UActivityComponent::TriggerGoalGenerator(
    TSubclassOf<UGoalGeneratorBase> GeneratorClass, AActor* Source)
{
    if (!NPCDefinition || !GeneratorClass) return;

    for (UGoalGeneratorBase* Gen : NPCDefinition->GoalGenerators)
    {
        if (!Gen || !Gen->IsA(GeneratorClass)) continue;

        UGoalItem* NewGoal = Gen->TryGenerateGoal(Source, this);
        if (NewGoal)
            AddGoal(NewGoal->GetClass(), Source);

        return; // one generator per class
    }
}

void UActivityComponent::SetRescoringInterval(float NewInterval)
{
    RescoringInterval = FMath::Max(NewInterval, 0.05f);
}

void UActivityComponent::RequestImmediateRescore()
{
    bNeedsImmediateRescore = true;
}

void UActivityComponent::OnOwnerDied()
{
    bIsDead = true;
    ClearAllGoals(EGoalRemoveReason::OwnerDied);

    if (AAIC_NPCController* AIC = GetCachedController())
    {
        if (AIC->BrainComponent)
            AIC->BrainComponent->StopLogic(TEXT("Owner died"));
    }

    ActiveActivity = nullptr;
    OnActivityChanged.Broadcast(nullptr, nullptr);
}

void UActivityComponent::CleanupExpiredGoals(float DeltaTime)
{
    for (int32 i = GoalList.Num() - 1; i >= 0; i--)
    {
        UGoalItem* Goal = GoalList[i];
        if (!Goal) { GoalList.RemoveAtSwap(i); continue; }

        Goal->TickLifetime(DeltaTime);

        if (Goal->HasExpired())
        {
            Goal->OnRemoved(EGoalRemoveReason::Expired);
            GoalList.RemoveAtSwap(i);
            bNeedsImmediateRescore = true;
        }
    }
}

void UActivityComponent::Rescore()
{
    UActivityBase* Best = SelectBestActivity();
    if (!Best || Best == ActiveActivity) return;

    if (ActiveActivity && !ActiveActivity->bIsInterruptible) return;

    SwitchToActivity(Best);
}

UActivityBase* UActivityComponent::SelectBestActivity()
{
    if (!NPCDefinition) return nullptr;

    UActivityBase* BestActivity = nullptr;
    float          BestScore = -1.f;

    for (UActivityBase* Activity : NPCDefinition->Activities)
    {
        if (!Activity || !Activity->CanActivate(this)) continue;

        UGoalItem* BestGoal = Activity->FindBestCompatibleGoal(
            reinterpret_cast<TArray<UGoalItem*>&>(GoalList));

        if (!BestGoal) continue;

        float Score = Activity->ScoreActivity(this)
            * BestGoal->GetGoalScore();

        if (Score > BestScore)
        {
            BestScore = Score;
            BestActivity = Activity;
        }
    }

    return BestActivity;
}

void UActivityComponent::SwitchToActivity(UActivityBase* NewActivity)
{
    UActivityBase* OldActivity = ActiveActivity;

    if (OldActivity)
    {
        OldActivity->OnActivityEnded(this);

        if (AAIC_NPCController* AIC = GetCachedController())
        {
            if (AIC->BrainComponent)
                AIC->BrainComponent->StopLogic(TEXT("Activity switched"));
        }
    }

    ActiveActivity = NewActivity;
    if (!NewActivity) return;

    if (AAIC_NPCController* AIC = GetCachedController())
    {
        if (NewActivity->BlackboardData && NewActivity->BehaviorTree)
        {
            UBlackboardComponent* BB = nullptr;
            AIC->UseBlackboard(NewActivity->BlackboardData, BB);

            if (BB)
                NewActivity->SetupBlackboard(BB, this);

            AIC->RunBehaviorTree(NewActivity->BehaviorTree);
        }
    }

    NewActivity->OnActivityStarted(this);
    OnActivityChanged.Broadcast(OldActivity, NewActivity);
}

UGoalItem* UActivityComponent::FindExistingGoal(
    TSubclassOf<UGoalItem> GoalClass) const
{
    for (UGoalItem* Goal : GoalList)
        if (Goal && Goal->IsA(GoalClass)) return Goal;
    return nullptr;
}

AAIC_NPCController* UActivityComponent::GetCachedController()
{
    if (!CachedController)
    {
        if (APawn* Pawn = Cast<APawn>(GetOwner()))
            CachedController = Cast<AAIC_NPCController>(
                Pawn->GetController());
    }
    return CachedController;
}