#include "AbilityUserComponent.h"
#include "../Helpers.h"

//
// AbilityUserComponent
//

UAbilityUserComponent::UAbilityUserComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}



void UAbilityUserComponent::BeginPlay()
{
	Super::BeginPlay();
}



/// <summary>
/// Intended to be called at BeginPlay of the character blueprint to add an ability to the abilityUserComponent.
/// </summary>
/// <param name="abilityToAdd">Pointer to the ability actor</param>
/// <param name="abilityUIIcon">UI icon to use for the ability(Doesnt need to be valid)</param>
void UAbilityUserComponent::AddAbility(UChildActorComponent* abilityToAdd, UTexture2D* abilityUIIcon)
{
	FAbilityInfo newAbilityInfo{};
	AAbilityBase* newAbility = nullptr;
	if (abilityToAdd != nullptr)
	{
		newAbility = Cast<AAbilityBase>(abilityToAdd->GetChildActor());
		check(newAbility != nullptr);
	}

	newAbilityInfo.m_pAbilityRef = newAbility;
	newAbilityInfo.m_pUIIcon = abilityUIIcon;

	m_KnownAbilities.Add(newAbilityInfo);
}



/// <summary>
/// Overwrites the abilitiy at the given index with the given one.
/// </summary>
/// <param name="index">Index to overwrite</param>
/// <param name="newAbility"></param>
void UAbilityUserComponent::SetAbilityAtIndex(int index, FAbilityInfo newAbility)
{
	if (!IsValidIndex(index, m_KnownAbilities))
		return LogText(ELogVerbosity::Warning, "UAbilityUserComponent::SetAbilityAtIndex tried to set new ability at invalid index[ " + FString::FromInt(index) + " ]");

	m_KnownAbilities[index] = newAbility;
}



/// <summary>
/// Checks if the given index is usable.
/// </summary>
/// <param name="index">Index to check</param>
/// <returns>If the ability index is valid</returns>
bool UAbilityUserComponent::AbilityPreCheck(uint32 index) const
{
	if (!IsValidIndex(index, m_KnownAbilities))
		return false;

	if (m_KnownAbilities[index].m_pAbilityRef == nullptr)
		return false;

	return true;
}



/// <summary>
/// Intended to be called when given input on the player or trough the AI behavior tree.
/// Passes a reference to the owning actor to use in the ability.
/// </summary>
/// <param name="index">Index of the ability to use</param>
void UAbilityUserComponent::UseAbility(int index)
{
	if (!AbilityPreCheck(index))
		return LogText(ELogVerbosity::Warning, "UAbilityUserComponent::UseAbility preCheck failed!");

	ensure(this->GetOwner() != nullptr); // the component is assumed to always have an owning actor!
	m_KnownAbilities[index].m_pAbilityRef->UseAbility(this->GetOwner());
}



FAbilityInfo UAbilityUserComponent::GetAbilityInfo(int index) const
{
	return (AbilityPreCheck(index)) ? m_KnownAbilities[index] : FAbilityInfo();
}

