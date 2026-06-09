#include "LocomotionTags.h"

namespace LocomotionTags 
{
	// Gait
		UE_DEFINE_GAMEPLAY_TAG(Gait_Walk, "LocomotionSystem.Gait.Walk")
		UE_DEFINE_GAMEPLAY_TAG(Gait_Sprint, "LocomotionSystem.Gait.Sprint")
		UE_DEFINE_GAMEPLAY_TAG(Gait_Run, "LocomotionSystem.Gait.Run")

	// Stance
		UE_DEFINE_GAMEPLAY_TAG(Stance_Stand, "LocomotionSystem.Stance.Stand")
		UE_DEFINE_GAMEPLAY_TAG(Stance_Crouch, "LocomotionSystem.Stance.Crouch")

	// Character State
		UE_DEFINE_GAMEPLAY_TAG(CharacterState_Relaxed, "LocomotionSystem.CharacterState.Relaxed")
		UE_DEFINE_GAMEPLAY_TAG(CharacterState_Stealth, "LocomotionSystem.CharacterState.Stealth")
		UE_DEFINE_GAMEPLAY_TAG(CharacterState_Combat, "LocomotionSystem.CharacterState.Combat")

	// Movement State
		UE_DEFINE_GAMEPLAY_TAG(MovementState_Idle, "LocomotionSystem.MovementState.Idle")
		UE_DEFINE_GAMEPLAY_TAG(MovementState_Moving, "LocomotionSystem.MovementState.Moving")

	// Movement Mode
		UE_DEFINE_GAMEPLAY_TAG(MovementMode_OnGround, "LocomotionSystem.MovementMode.OnGround")
		UE_DEFINE_GAMEPLAY_TAG(MovementMode_InAir, "LocomotionSystem.MovementMode.InAir")
		UE_DEFINE_GAMEPLAY_TAG(MovementMode_Ragdoll, "LocomotionSystem.MovementMode.Ragdoll")
}