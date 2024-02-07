// Fill out your copyright notice in the Description page of Project Settings.
// Copyright 2024 QinXiao, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SpinBox.h"
#include "CustomSpinBox.generated.h"

/**
 * 
 */
UCLASS()
class REFLECTIONTOOL_API UCustomSpinBox : public USpinBox
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "ReflectionTool|HelperUMG")
	void SetTextSize(int32 NewSize);
};
