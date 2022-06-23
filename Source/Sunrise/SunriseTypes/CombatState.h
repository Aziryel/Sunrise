#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ESC_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ESC_Reloading UMETA(DisplayName = "Reloading"),
	ESC_ThrowingGrenade UMETA(DisplayName = "Throwing Greande"),

	ESC_MAX UMETA(DisplayName = "DefaultMAX")
};