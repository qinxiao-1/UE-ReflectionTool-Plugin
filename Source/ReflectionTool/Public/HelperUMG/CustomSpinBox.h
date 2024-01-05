// Fill out your copyright notice in the Description page of Project Settings.

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
	UFUNCTION(BlueprintCallable)
	void SetTextSize(int32 NewSize);
};
