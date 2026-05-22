#include "AI/StateTree/STCondition_HasTarget.h"
#include "StateTreeExecutionContext.h"
#include "Character/ARPGCharacterBase.h"

bool FSTCondition_HasTarget::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	AActor* Target = InstanceData.TargetActor;
	if (!Target)
	{
		return false;
	}

	if (InstanceData.bCheckAlive)
	{
		const AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(Target);
		if (CharBase && CharBase->IsDead())
		{
			return false;
		}
	}

	return true;
}
