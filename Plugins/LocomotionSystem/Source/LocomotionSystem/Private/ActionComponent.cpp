#include "ActionComponent.h"
#include "Goal_Generator.h"

UActionComponent::UActionComponent()
{
	// CRITICAL FIX: Torn down empty TickComponent boilerplate completely.
	PrimaryComponentTick.bCanEverTick = false;
}

void UActionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ActivityConfiguration)
	{
		for (TSubclassOf<UActivity_Base> ActClass : ActivityConfiguration->ActivityCapabilities)
		{
			if (ActClass)
			{
				UActivity_Base* NewAct = NewObject<UActivity_Base>(GetOwner(), ActClass);
				InstantiatedActivities.Add(NewAct);
			}
		}

		for (TSubclassOf<UGoal_Generator> GenClass : ActivityConfiguration->GoalGenerators)
		{
			if (GenClass)
			{
				UGoal_Generator* NewGen = NewObject<UGoal_Generator>(GetOwner(), GenClass);
				InstantiatedGenerators.Add(NewGen);
			}
		}

		if (InstantiatedGenerators.Num() > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(
				RescoreTimerHandle,
				this,
				&UActionComponent::RunGoalGenerationPass,
				ActivityConfiguration->RescoreInterval,
				true
			);
		}
	}
}

void UActionComponent::AddGoal(UGoal_Base* NewGoal)
{
	if (!NewGoal || ActiveGoals.Contains(NewGoal)) return;
	ActiveGoals.Add(NewGoal);
	EvaluateHighestBidder();
}

void UActionComponent::RemoveGoal(UGoal_Base* GoalToRemove)
{
	if (!GoalToRemove || !ActiveGoals.Contains(GoalToRemove)) return;
	ActiveGoals.Remove(GoalToRemove);
	EvaluateHighestBidder();
}

void UActionComponent::EvaluateHighestBidder()
{
	// OPTIMIZATION & PROTECTION GATE: Respect active activity interruption constraints
	if (CurrentActiveActivity && !CurrentActiveActivity->bIsInterruptible)
	{
		return;
	}

	UGoal_Base* BestGoal = nullptr;
	UActivity_Base* BestActivity = nullptr;
	float HighestCombinedScore = -1.0f;

	for (int32 i = ActiveGoals.Num() - 1; i >= 0; --i)
	{
		if (!ActiveGoals[i] || !ActiveGoals[i]->IsGoalValid(GetOwner()))
		{
			ActiveGoals.RemoveAt(i);
			continue;
		}

		float GoalScore = ActiveGoals[i]->ScoreGoal(GetOwner());
		float ActivityScore = 0.0f;

		UActivity_Base* MatchingActivity = FindBestActivityForGoal(ActiveGoals[i], ActivityScore);

		if (MatchingActivity)
		{
			float CombinedScore = GoalScore + ActivityScore;
			if (CombinedScore > HighestCombinedScore)
			{
				HighestCombinedScore = CombinedScore;
				BestGoal = ActiveGoals[i];
				BestActivity = MatchingActivity;
			}
		}
	}

	if (BestGoal != CurrentActiveGoal)
	{
		// CRITICAL FIX: Notify and shut down the previously active state safely before running the next
		if (CurrentActiveActivity && CurrentActiveGoal)
		{
			CurrentActiveActivity->EndActivity(GetOwner(), CurrentActiveGoal, false);
		}

		CurrentActiveGoal = BestGoal;
		CurrentActiveActivity = BestActivity;

		if (CurrentActiveGoal && CurrentActiveActivity)
		{
			CurrentActiveActivity->RunActivity(GetOwner(), CurrentActiveGoal);
			OnGoalExecutionSwapped.Broadcast(CurrentActiveGoal, CurrentActiveActivity, CurrentActiveActivity->bIsInterruptible);
		}
		else
		{
			CurrentActiveGoal = nullptr;
			CurrentActiveActivity = nullptr;
			OnGoalExecutionSwapped.Broadcast(nullptr, nullptr, true);
		}
	}
}

UActivity_Base* UActionComponent::FindBestActivityForGoal(UGoal_Base* Goal, float& OutCombinedScore)
{
	if (!Goal) return nullptr;

	UActivity_Base* SelectedActivity = nullptr;
	float BestActivityScore = -1.0f;

	for (UActivity_Base* Activity : InstantiatedActivities)
	{
		if (Activity && Activity->SupportedGoalClass && Goal->IsA(Activity->SupportedGoalClass))
		{
			if (Activity->CanRunActivity(GetOwner(), Goal))
			{
				float ActionScore = Activity->ScoreActivity(GetOwner(), Goal);
				float GoalItemScore = Activity->ScoreGoalItem(GetOwner(), Goal);
				float Aggregate = ActionScore + GoalItemScore;

				if (Aggregate > BestActivityScore)
				{
					BestActivityScore = Aggregate;
					SelectedActivity = Activity;
				}
			}
		}
	}

	OutCombinedScore = BestActivityScore;
	return SelectedActivity;
}

void UActionComponent::RunGoalGenerationPass()
{
	for (UGoal_Generator* Generator : InstantiatedGenerators)
	{
		if (Generator)
		{
			Generator->EvaluateAndGenerateGoals(GetOwner(), this);
		}
	}
	EvaluateHighestBidder();
}

void UActionComponent::ForceResetComponentState()
{
	if (CurrentActiveActivity && CurrentActiveGoal)
	{
		CurrentActiveActivity->EndActivity(GetOwner(), CurrentActiveGoal, false);
	}

	ActiveGoals.Empty();
	CurrentActiveGoal = nullptr;
	CurrentActiveActivity = nullptr;
}

void UActionComponent::ReevaluateStatePostLoad()
{
	EvaluateHighestBidder();
}