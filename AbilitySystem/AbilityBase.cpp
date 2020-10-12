#include "AbilityBase.h"

//
// AbilityBase
//

AAbilityBase::AAbilityBase()
{
	PrimaryActorTick.bCanEverTick = true;
}



void AAbilityBase::BeginPlay()
{
	Super::BeginPlay();
}



void AAbilityBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// when ability was casted after set "cooldown" be able to cast ability again
	if (m_CanUseAbility == false)
	{
		if ((m_TimeSinceAbilityUsed += DeltaTime) >= m_Cooldown)
		{
			m_CanUseAbility = true;
			m_TimeSinceAbilityUsed = m_Cooldown;
			OnAbilityCooldownEnded();	// implementable even in bleuprint graph
		}
	}
}



#if WITH_EDITOR
/// <summary>
/// Changes m_Cooldown to read only when the ability type is passive;
/// </summary>
bool AAbilityBase::CanEditChange(const FProperty* inProperty) const
{
	const bool bCanParrentChange = Super::CanEditChange(inProperty);

	// Can we edit cast time
	if (inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AAbilityBase, m_Cooldown))
	{
		return m_Type == EAbilityType::ACTIVE;
	}

	return bCanParrentChange;
}
#endif // WITH_EDITOR



/// <summary>
/// Should be called when casting an ability to reset the "m_TimeSinceAbilityUsed" variable
/// </summary>
void AAbilityBase::CastedAbility()
{
	check(m_TimeSinceAbilityUsed >= m_Cooldown);
	check(m_Type == EAbilityType::ACTIVE);
	m_CanUseAbility = false;
	m_TimeSinceAbilityUsed = 0.0f;
}



bool AAbilityBase::CanCastAbility() const
{
	return m_CanUseAbility;
}



/// <summary>
/// gives the cooldown percentage or 1.0f for UI purposes
/// </summary>
/// <returns>a value from 1.0 to 0.0 with 0.0 meaning you can use the ability again</returns>
float AAbilityBase::CooldownPercentage() const
{
	check(m_Cooldown != 0.0f); // cant devide by 0
	return (m_Cooldown != 0.0f) ? m_TimeSinceAbilityUsed / m_Cooldown : 1.0f;
}



//
// AbilityInfo
//

FAbilityInfo::FAbilityInfo()
{

}