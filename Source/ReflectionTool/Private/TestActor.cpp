// Fill out your copyright notice in the Description page of Project Settings.


#include "TestActor.h"

#include "ReflectionToolLib.h"

// Sets default values
ATestActor::ATestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATestActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATestActor::Test()
{
	int32 a = 0;
	TTuple<int32> T;
	UReflectionToolLib::InvokeFunctionByName(this, "TestFloatToInt", T, 3.2f);
	a = T.Get<0>();
	UE_LOG(LogTemp, Warning, TEXT("%d"), a);
}

int32 ATestActor::TestFloatToInt(float a, int& out)
{
	out = static_cast<int>(a);
	return a;
}

void ATestActor::TestFloat(float a)
{
	UE_LOG(LogTemp, Warning, TEXT("%.2f"), a);
}

