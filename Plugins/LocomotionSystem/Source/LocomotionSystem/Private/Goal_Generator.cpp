#include "Goal_Generator.h"

bool UGoal_Generator::IsOnCooldown(UWorld* WorldContext) const
{
	if (!WorldContext) return false;
	return (WorldContext->GetTimeSeconds() - LastExecutionTime) < CooldownDuration;
}

void UGoal_Generator::TriggerCooldown(UWorld* WorldContext)
{
	if (WorldContext)
	{
		LastExecutionTime = WorldContext->GetTimeSeconds();
	}
}