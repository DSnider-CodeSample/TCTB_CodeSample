// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CheckPursueStatus.generated.h"

UCLASS()
class SPOOKYGAME_API UBTService_CheckPursueStatus : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_CheckPursueStatus();

	/**
	 * The logic of this service. Handles checking for state changes and updating variables.
	 * Runs every tick that this servie is active in the behavior tree
	 * @param OwnerComp Behavior tree owning this service
	 * @param NodeMemory NodeMemory
	 * @param DeltaSeconds Time since last tick in seconds
	 */
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;	
};
