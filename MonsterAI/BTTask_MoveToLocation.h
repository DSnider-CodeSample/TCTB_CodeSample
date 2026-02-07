// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_MoveToLocation.generated.h"

UCLASS()
class SPOOKYGAME_API UBTTask_MoveToLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	/**
	 * Makes the monster move to a pre-selected location accessed through blackboard.
	 * Executes this task when called
	 * @param OwnerComp Behavior tree owning this service
	 * @param NodeMemory 
	 * @return 
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;	
};
