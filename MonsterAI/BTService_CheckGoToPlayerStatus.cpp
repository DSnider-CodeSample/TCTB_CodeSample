// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_CheckGoToPlayerStatus.h"

#include "Monster.h"
#include "MonsterAIController.h"
#include "MonsterStates.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Kismet/GameplayStatics.h"

UBTService_CheckGoToPlayerStatus::UBTService_CheckGoToPlayerStatus()
{
	bCreateNodeInstance = true;
}

void UBTService_CheckGoToPlayerStatus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	//get the AIController for the monster
	AMonsterAIController* AIController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	//set target location to the player location
	OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID, UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetActorLocation());

	//increment time since pursue
	const float GoToPlayerTotalTime = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Float>(AIController->GoToPlayerTotalTimeKeyID);
	OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->GoToPlayerTotalTimeKeyID, GoToPlayerTotalTime + DeltaSeconds);

	//switch to search mode
	if (GoToPlayerTotalTime >= OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Float>(AIController->HowLongToGoToPlayerKeyID))
	{
		//get distance to current target point
		const FVector Target = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID);
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
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->GoToPlayerTotalTimeKeyID, 0);
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(AIController->SearchCenterPointKeyID, AIController->GetPawn()->GetActorLocation());
		}
		else
		{
			//pursue the player's last position if the monster wasn't fast enough to actually chase the player within the time limit
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Enum>(AIController->StateKeyID, EGurneyMonsterStates::GMS_Pursue);
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->GoToPlayerTotalTimeKeyID, 0);
		}
	}
}
