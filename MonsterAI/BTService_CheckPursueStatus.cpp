// Fill out your copyright notice in the Description page of Project Settings.

#include "BTService_CheckPursueStatus.h"

#include "Monster.h"
#include "MonsterAIController.h"
#include "MonsterStates.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTService_CheckPursueStatus::UBTService_CheckPursueStatus()
{
	bCreateNodeInstance = true;
}

void UBTService_CheckPursueStatus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	//get the AIController for the monster
	AMonsterAIController* AIController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());	

	//create vectors to use in the GetRandomReachablePointInRadius
	const FVector Target = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID);

	//get distance to current target point
	const float Distance = FVector::Distance(AIController->GetPawn()->GetActorLocation(), Target);

	//if within a tolerable radius of the target location...
	if (Distance < 150)
	{
		if (OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Enum>(AIController->StateKeyID) != EGurneyMonsterStates::GMS_Search)
		{
			// play transition sound
			AMonster* Monster = Cast<AMonster>(AIController->GetPawn());
			Monster->PlayMonsterSound("PlayMonsterSearchLoop");
		}

		//set state to search
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Enum>(AIController->StateKeyID, EGurneyMonsterStates::GMS_Search);
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->WanderRadiusKeyID, AIController->GetDesiredSearchRadius());
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID, AIController->GetPawn()->GetActorLocation());
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->TimeSincePursueKeyID, 0.0f);
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(AIController->SearchCenterPointKeyID, AIController->GetPawn()->GetActorLocation());
	}
}
