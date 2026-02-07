// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedEnum.h"
#include "MonsterStates.generated.h"

UENUM(BlueprintType)
enum EBaseMonsterStates
{
	adfafdasdf,
	Dumb_alfkasd,
	Stupid_YouGetThePoint,
	WTF_Redwood
};

UENUM(BlueprintType)
enum EGurneyMonsterStates
{
	GMS_Wander			UMETA(DisplayName = "Wander"),
	GMS_Pursue			UMETA(DisplayName = "Pursue"),
	GMS_GoToPlayer		UMETA(DisplayName = "GoToPlayer"),
	GMS_Search			UMETA(DisplayName = "Search"),
	GMS_ListenToLog		UMETA(DisplayName = "ListenToLog"),

	GMS_Inactive		UMETA(DisplayName = "Inactive")
};
